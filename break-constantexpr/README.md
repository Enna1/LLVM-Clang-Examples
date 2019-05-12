# BreakConstantExpr

This BreakConstantExpr module pass convert all constant expressions of a module into instructions.  

This routine does ***not*** perform any recursion, so the resulting instruction may have constant expression operands.

This BreakConstantExpr module pass is extracted from "The SAFECode Compiler" project.