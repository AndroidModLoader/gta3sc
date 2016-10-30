// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 | %FileCheck %s
VAR_INT n

// Using a default case, and an out of order case.
{
    // CHECK-L: SWITCH_START &8 4i8 1i8 @MAIN_5 50i8 @MAIN_4 100i8 @MAIN_1 200i16 @MAIN_2 300i16 @MAIN_3 -1i8 @MAIN_6 -1i8 @MAIN_6 -1i8 @MAIN_6
    SWITCH n
        // CHECK-NEXT-L: MAIN_1:
        CASE 100
            // CHECK-NEXT-L: WAIT 100i8
            WAIT 100
            // CHECK-NEXT-L: GOTO @MAIN_6
            BREAK
        // CHECK-NEXT-L: MAIN_2:
        CASE 200
            // CHECK-NEXT-L: WAIT 200i16
            WAIT 200
            // CHECK-NEXT-L: GOTO @MAIN_6
            BREAK
        // CHECK-NEXT-L: MAIN_3:
        CASE 300
            // CHECK-NEXT-L: WAIT 300i16
            WAIT 300
            // CHECK-NEXT-L: GOTO @MAIN_6
            BREAK
        // CHECK-NEXT-L: MAIN_4:
        CASE 50
            // CHECK-NEXT-L: WAIT 50i8
            WAIT 50
            // CHECK-NEXT-L: GOTO @MAIN_6
            BREAK
        // CHECK-NEXT-L: MAIN_5:
        DEFAULT
            // CHECK-NEXT-L: WAIT 0i8
            WAIT 0
            // CHECK-NEXT-L: GOTO @MAIN_6
            BREAK
    ENDSWITCH
}


// Using no default case, and an out of order case.
{
    // CHECK-NEXT-L: MAIN_6:
    // CHECK-NEXT-L: SWITCH_START &8 3i8 0i8 @MAIN_10 50i8 @MAIN_9 100i8 @MAIN_7 200i16 @MAIN_8 -1i8 @MAIN_10 -1i8 @MAIN_10 -1i8 @MAIN_10 -1i8 @MAIN_10
    SWITCH n
        // CHECK-NEXT-L: MAIN_7:
        CASE 100
            // CHECK-NEXT-L: WAIT 100i8
            WAIT 100
            // CHECK-NEXT-L: GOTO @MAIN_10
            BREAK
        // CHECK-NEXT-L: MAIN_8:
        CASE 200
            // CHECK-NEXT-L: WAIT 200i16
            WAIT 200
            // CHECK-NEXT-L: GOTO @MAIN_10
            BREAK
        // CHECK-NEXT-L: MAIN_9:
        CASE 50
            // CHECK-NEXT-L: WAIT 50i8
            WAIT 50
            // CHECK-NEXT-L: GOTO @MAIN_10
            BREAK
    ENDSWITCH
}

// Using over 7 cases, should generate a SWITCH_CONTINUED!
{
    // CHECK-NEXT-L: MAIN_10:
    // CHECK-NEXT-L: SWITCH_START &8 9i8 0i8 @MAIN_20 100i8 @MAIN_11 200i16 @MAIN_12 300i16 @MAIN_13 400i16 @MAIN_14 500i16 @MAIN_15 600i16 @MAIN_16 700i16 @MAIN_17
    // CHECK-NEXT-L: SWITCH_CONTINUED 800i16 @MAIN_18 900i16 @MAIN_19 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 
    SWITCH n
        // CHECK-NEXT-L: MAIN_11:
        CASE 100
            // CHECK-NEXT-L: WAIT 100i8 
            WAIT 100
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_12:
        CASE 200
            // CHECK-NEXT-L: WAIT 200i16 
            WAIT 200
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_13:
        CASE 300
            // CHECK-NEXT-L: WAIT 300i16 
            WAIT 300
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_14:
        CASE 400
            // CHECK-NEXT-L: WAIT 400i16 
            WAIT 400
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_15:
        CASE 500
            // CHECK-NEXT-L: WAIT 500i16 
            WAIT 500
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_16:
        CASE 600
            // CHECK-NEXT-L: WAIT 600i16 
            WAIT 600
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_17:
        CASE 700
            // CHECK-NEXT-L: WAIT 700i16 
            WAIT 700
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_18:
        CASE 800
            // CHECK-NEXT-L: WAIT 800i16 
            WAIT 800
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
        // CHECK-NEXT-L: MAIN_19:
        CASE 900
            // CHECK-NEXT-L: WAIT 900i16 
            WAIT 900
            // CHECK-NEXT-L: GOTO @MAIN_20 
            BREAK
    ENDSWITCH
}

// CHECK-NEXT-L: MAIN_20:
// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT
TERMINATE_THIS_SCRIPT