#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <objscip/objscip.h>
#include <objscip/objscipdefplugins.h>
#include <print>
#include <ranges>
#include <string>
#include <vector>

#include "utils.hpp"

constexpr int chessboard_size = 8;
constexpr int idx(int row, int col) { return row * chessboard_size + col; };

struct SCIPDeleter {
  void operator()(SCIP *scip) const { SCIPfree(&scip); }
};
using SCIPPtr = std::unique_ptr<SCIP, SCIPDeleter>;

struct VarDeleter {
  SCIP *scip;
  VarDeleter(SCIP *scip) : scip(scip) {}
  void operator()(SCIP_VAR *var) const { SCIPreleaseVar(scip, &var); }
};
using VarPtr = std::unique_ptr<SCIP_VAR, VarDeleter>;

struct ConstDeleter {
  SCIP *scip;
  ConstDeleter(SCIP *scip) : scip(scip) {}
  void operator()(SCIP_CONS *cons) const { SCIPreleaseCons(scip, &cons); }
};
using ConstPtr = std::unique_ptr<SCIP_CONS, ConstDeleter>;

// Adds  sum(vars) in [lhs, rhs]  as a linear constraint.
void add_linear_cons(SCIP *scip, std::string const &name,
                     std::vector<SCIP_VAR *> const &vars, SCIP_Real lhs,
                     SCIP_Real rhs) {
  ConstPtr cons(nullptr, ConstDeleter(scip));
  CALL_CHECK(SCIPcreateConsBasicLinear(scip, std::out_ptr(cons), name.c_str(),
                                       0, nullptr, nullptr, lhs, rhs));
  for (auto *var : vars) {
    CALL_CHECK(SCIPaddCoefLinear(scip, cons.get(), var, 1.0));
  }
  CALL_CHECK(SCIPaddCons(scip, cons.get()));
}

void add_no_good(SCIP *scip, std::vector<SCIP_Real> const &sol_values,
                 std::vector<VarPtr> const &vars, int solution_number) {
  ConstDeleter constDeleter(scip);
  ConstPtr cons(nullptr, constDeleter);
  CALL_CHECK(SCIPcreateConsBasicLinear(
      scip, std::out_ptr(cons),
      std::format("no_good_{}", solution_number).c_str(), 0, nullptr, nullptr,
      -SCIPinfinity(scip), 0.0));
  int num_selected = 0;
  for (int i = 0; i < chessboard_size; ++i) {
    for (int j = 0; j < chessboard_size; ++j) {
      if (sol_values[idx(i, j)] > 0.5) {
        ++num_selected;
        CALL_CHECK(
            SCIPaddCoefLinear(scip, cons.get(), vars[idx(i, j)].get(), 1.0));
      }
    }
  }
  // Forbid exactly this set of queens: at least one of them must change.
  CALL_CHECK(SCIPchgRhsLinear(scip, cons.get(), num_selected - 1.0));
  CALL_CHECK(SCIPaddCons(scip, cons.get()));
}

// Two solutions are the same iff the same cells carry a queen.
bool same_solution(std::vector<SCIP_Real> const &lhs,
                   std::vector<SCIP_Real> const &rhs) {
  for (const auto [a, b] : std::views::zip(lhs, rhs)) {
    if ((a > 0.5) != (b > 0.5)) {
      return false;
    }
  }
  return true;
}

std::string get_sol_string(SCIP *scip, SCIP_SOL *sol, auto var_at) {
  std::string line;
  for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
    for (const auto j : std::ranges::views::iota(0, chessboard_size)) {
      line += SCIPgetSolVal(scip, sol, var_at(i, j)) > 0.5 ? 'Q' : '.';
    }
    line += "\n";
  }
  return line;
}

int main() {
  SCIPPtr scip;
  CALL_CHECK(SCIPcreate(std::out_ptr(scip)));
  CALL_CHECK(SCIPincludeDefaultPlugins(scip.get()));
  CALL_CHECK(SCIPcreateProbBasic(scip.get(), "ExampleMIP"));
  const SCIP_Real inf = SCIPinfinity(scip.get());

  // Set objective sense
  CALL_CHECK(SCIPsetObjsense(scip.get(), SCIP_OBJSENSE_MAXIMIZE));
  std::vector<VarPtr> positions;
  VarDeleter varDeleter(scip.get());
  positions.reserve(chessboard_size * chessboard_size);
  for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
    for (const auto j : std::ranges::views::iota(0, chessboard_size)) {
      positions.push_back(VarPtr(nullptr, varDeleter));
      CALL_CHECK(SCIPcreateVarBasic(scip.get(), std::out_ptr(positions.back()),
                                    std::format("x{}_{}", i, j).c_str(), 0.0,
                                    1.0, 1.0, SCIP_VARTYPE_BINARY));
      CALL_CHECK(SCIPaddVar(scip.get(), positions.back().get()));
    }
  }

  const auto var_at = [&positions](int row, int col) {
    return positions[idx(row, col)].get();
  };

  // At most one queen per row.
  for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
    std::vector<SCIP_VAR *> row;
    for (const auto j : std::ranges::views::iota(0, chessboard_size)) {
      row.push_back(var_at(i, j));
    }
    add_linear_cons(scip.get(), std::format("row_{}", i), row, -inf, 1.0);
  }

  // At most one queen per column.
  for (const auto j : std::ranges::views::iota(0, chessboard_size)) {
    std::vector<SCIP_VAR *> col;
    for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
      col.push_back(var_at(i, j));
    }
    add_linear_cons(scip.get(), std::format("col_{}", j), col, -inf, 1.0);
  }

  // At most one queen per "\" diagonal, indexed by d = row - col.
  for (const auto d :
       std::ranges::views::iota(-(chessboard_size - 1), chessboard_size)) {
    std::vector<SCIP_VAR *> diag;
    for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
      const int j = i - d;
      if (j >= 0 && j < chessboard_size) {
        diag.push_back(var_at(i, j));
      }
    }
    if (diag.size() < 2) {
      continue; // corner cells: constraint is implied by the bounds
    }
    add_linear_cons(scip.get(), std::format("diag_{}", d), diag, -inf, 1.0);
  }

  // At most one queen per "/" diagonal, indexed by s = row + col.
  for (const auto s : std::ranges::views::iota(0, 2 * chessboard_size - 1)) {
    std::vector<SCIP_VAR *> anti_diag;
    for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
      const int j = s - i;
      if (j >= 0 && j < chessboard_size) {
        anti_diag.push_back(var_at(i, j));
      }
    }
    if (anti_diag.size() < 2) {
      continue;
    }
    add_linear_cons(scip.get(), std::format("anti_diag_{}", s), anti_diag, -inf,
                    1.0);
  }
  std::string full_sol_string;
  std::vector<std::vector<SCIP_Real>> all_sol_values;
  for (const auto i : std::ranges::views::iota(0, chessboard_size)) {
    CALL_CHECK(SCIPsolve(scip.get()));
    SCIP_SOL *sol = SCIPgetBestSol(scip.get());
    if (sol == nullptr) {
      std::println("no further solution found after {} solution(s)", i);
      break;
    }
    full_sol_string += std::format("i:{}\n", i);
    full_sol_string += get_sol_string(scip.get(), sol, var_at);

    // Copy the values out: the solution is owned by the transformed problem,
    // which we free below so that constraints can be added again.
    std::vector<SCIP_Real> sol_values;
    sol_values.reserve(positions.size());
    for (const auto &var : positions) {
      sol_values.push_back(SCIPgetSolVal(scip.get(), sol, var.get()));
    }

    // The no-good cuts should make every solution distinct; verify that.
    for (const auto k :
         std::ranges::views::iota(std::size_t{0}, all_sol_values.size())) {
      if (same_solution(all_sol_values[k], sol_values)) {
        std::println(std::cerr, "solution {} repeats solution {}", i, k);
        return 1;
      }
    }
    all_sol_values.push_back(sol_values);

    CALL_CHECK(SCIPfreeTransform(scip.get()));
    add_no_good(scip.get(), sol_values, positions, i);
  }
  std::print("{}", full_sol_string);
  return 0;
}
