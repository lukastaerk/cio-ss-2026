#include <format>
#include <iostream>
#include <memory>
#include <objscip/objscip.h>
#include <objscip/objscipdefplugins.h>
#include <print>
#include <vector>

#include "utils.hpp"

// Wrapper for SCIP -> SCIPPtr and SCIPDeleter

// Wrapper for SCIP_VAR -> VarPtr and VarDeleter

// Wrapper for SCIP_CONS -> ConstPtr and ConstDeleter

int main(){
    // Create SCIP pointer, initialize and set up the problem
    
    //Set objective sense

    // Create Variables
    
    // Create Constraints

    // Print problem as "cip" to stdout for debug

    // Solve the problem

    // Check if the solution is optimal, print the optimal objective value along with the solution 
    
}