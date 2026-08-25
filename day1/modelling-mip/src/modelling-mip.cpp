#include <format>
#include <iostream>
#include <memory>
#include <objscip/objscip.h>
#include <objscip/objscipdefplugins.h>
#include <print>
#include <vector>

#include "utils.hpp"

struct SCIPDeleter{
    void operator()(SCIP* scip) const{
        SCIPfree(&scip);
    }
};
using SCIPPtr = std::unique_ptr<SCIP, SCIPDeleter>;

struct VarDeleter{
    SCIP* scip;
    VarDeleter(SCIP* scip): scip(scip){}
    void operator()(SCIP_VAR* var) const{
        SCIPreleaseVar(scip, &var);
    }
};
using VarPtr = std::unique_ptr<SCIP_VAR, VarDeleter>;

struct ConstDeleter{
    SCIP* scip;
    ConstDeleter(SCIP* scip): scip(scip){}
    void operator()(SCIP_CONS* cons) const{
        SCIPreleaseCons(scip, &cons);
    }
};
using ConstPtr = std::unique_ptr<SCIP_CONS, ConstDeleter>;

int main(){
    SCIPPtr scip;
    CALL_CHECK(SCIPcreate(std::out_ptr(scip)));
    CALL_CHECK(SCIPincludeDefaultPlugins(scip.get()));
    CALL_CHECK(SCIPcreateProbBasic(scip.get(), "ExampleMIP"));
    const SCIP_Real inf = SCIPinfinity(scip.get());

    //Set objective sense
    CALL_CHECK(SCIPsetObjsense(scip.get(), SCIP_OBJSENSE_MAXIMIZE));

    // Create Variables
    VarDeleter varDeleter(scip.get());
    SCIP_Real obj[] = {1,2,3,1};
    SCIP_Real lb[] = {0,0,0,2};
    SCIP_Real ub[] = {40,inf,inf,3};
    SCIP_VARTYPE vartype[] = {SCIP_VARTYPE_CONTINUOUS, SCIP_VARTYPE_CONTINUOUS, SCIP_VARTYPE_CONTINUOUS, SCIP_VARTYPE_INTEGER};
    std::vector<VarPtr> vars;
    vars.reserve(4);
    for(int i= 0; i < 4; ++i){
        VarPtr var(nullptr, varDeleter);
        CALL_CHECK(SCIPcreateVarBasic(scip.get(), std::out_ptr(var), std::format("x{}", i).c_str(), lb[i], ub[i], obj[i], vartype[i]));
        CALL_CHECK(SCIPaddVar(scip.get(), var.get()));
        vars.push_back(std::move(var));
    }

    // Create Constraints
    std::vector<std::vector<int>> coeffind = {{1,2,3,4},{1,2,3},{2,4}};
    std::vector<std::vector<SCIP_Real>> coeffs = {{-1,1,1,10},{1,-3,1},{1,-3.5}};
    std::vector<SCIP_Real> lhs = {-inf, -inf, 0};
    std::vector<SCIP_Real> rhs = {20, 30, 0};
    ConstDeleter constDeleter(scip.get());
    for(int i =0; i < 3; ++i){
        ConstPtr cons(nullptr, constDeleter);
        CALL_CHECK(SCIPcreateConsBasicLinear(scip.get(), std::out_ptr(cons), std::format("c{}", i).c_str(),0, nullptr, nullptr, lhs[i], rhs[i]));
        for (int j = 0; j < std::ssize(coeffind[i]); ++j) {
            CALL_CHECK(SCIPaddCoefLinear(scip.get(), cons.get(), vars[coeffind[i][j]-1].get(), coeffs[i][j]));
        }
        CALL_CHECK(SCIPaddCons(scip.get(), cons.get()));
    }
    CALL_CHECK(SCIPprintOrigProblem(scip.get(), nullptr, "cip", false));
    CALL_CHECK(SCIPsolve(scip.get()));
    if (SCIPgetStatus(scip.get()) == SCIP_STATUS_OPTIMAL) {
        auto sol = SCIPgetBestSol(scip.get());
        std::cout << "Optimal solution found:\n";
        std::println("Objective value: {}",SCIPgetSolOrigObj(scip.get(), sol));
        std::cout << "Solution: \n"; 
        for (int i = 0; i < 4; ++i) {
            std::println("x{} = {}", i, SCIPgetSolVal(scip.get(), sol, vars[i].get()));
        }
    }
}