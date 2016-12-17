// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - | %FileCheck %s

VAR_INT i
VAR_FLOAT f

// CHECK-L: WAIT 101i8
WAIT my_int_const

// CHECK-L: SET_TIME_SCALE 0x1.800000p+0f
SET_TIME_SCALE my_flt_const

// CHECK-L: SET_VAR_INT &8 101i8
i = MY_INT_CONST

// CHECK-L: SET_VAR_FLOAT &12 0x1.800000p+0f
f = MY_FLT_CONST

// CHECK-NOT: CONST_INT.*
CONST_INT my_int_const 101

// CHECK-NOT: CONST_FLOAT.*
CONST_FLOAT my_flt_const 1.5

TERMINATE_THIS_SCRIPT