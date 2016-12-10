// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - | %FileCheck %s

GOSUB main_invoke
GOSUB main_stuff
TERMINATE_THIS_SCRIPT

main_stuff:
{
    PRINT_HELP main

    main_label1:
    IS_BUTTON_PRESSED PAD1 RIGHTSHOCK
    GOTO_IF_FALSE main_label2
    GOTO main_label3

    main_label2:
    WAIT 0
    GOTO main_label1

    main_label3:
    RETURN
}

main_invoke:
{
    // TODO GOSUB_FILE gosub1 gosub1.sc
    LAUNCH_MISSION subscript1.sc
    LOAD_AND_LAUNCH_MISSION mission1.sc
    REGISTER_STREAMED_SCRIPT stream1 stream1.sc
    RETURN
}

// TODO test gosubs into main_sublabel from a mission / streamed script
main_sublabel:
RETURN

// offsets.sc must use global offsets
// CHECK-L:      PRINT_HELP 'MAIN'
// CHECK-NEXT-L: MAIN_2:
// CHECK-NEXT-L: IS_BUTTON_PRESSED 0i8 19i8
// CHECK-NEXT-L: GOTO_IF_FALSE @MAIN_3
// CHECK-NEXT-L: GOTO @MAIN_4
// CHECK-NEXT-L: MAIN_3:
// CHECK-NEXT-L: WAIT 0i8
// CHECK-NEXT-L: GOTO @MAIN_2
// CHECK-NEXT-L: MAIN_4:
// CHECK-NEXT-L: RETURN

// subscript1.sc must use global offsets
// CHECK-L:      PRINT_HELP 'SUBSC1'
// CHECK-NEXT-L: MAIN_7:
// CHECK-NEXT-L: IS_BUTTON_PRESSED 0i8 19i8
// CHECK-NEXT-L: GOTO_IF_FALSE @MAIN_8
// CHECK-NEXT-L: GOTO @MAIN_9
// CHECK-NEXT-L: MAIN_8:
// CHECK-NEXT-L: WAIT 0i8
// CHECK-NEXT-L: GOTO @MAIN_7
// CHECK-NEXT-L: MAIN_9:
// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT

// mission1.sc must use local offsets
// CHECK-L:      PRINT_HELP 'MISS1'
// CHECK-NEXT-L: MISSION_0_1:
// CHECK-NEXT-L: IS_BUTTON_PRESSED 0i8 19i8
// CHECK-NEXT-L: GOTO_IF_FALSE %MISSION_0_2
// CHECK-NEXT-L: GOTO %MISSION_0_3
// CHECK-NEXT-L: MISSION_0_2:
// CHECK-NEXT-L: WAIT 0i8
// CHECK-NEXT-L: GOTO %MISSION_0_1
// CHECK-NEXT-L: MISSION_0_3:
// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT

// stream1.sc must use local offsets
// CHECK-L:      PRINT_HELP 'STREAM1'
// CHECK-NEXT-L: STREAM_0_1:
// CHECK-NEXT-L: IS_BUTTON_PRESSED 0i8 19i8
// CHECK-NEXT-L: GOTO_IF_FALSE %STREAM_0_2
// CHECK-NEXT-L: GOTO %STREAM_0_3
// CHECK-NEXT-L: STREAM_0_2:
// CHECK-NEXT-L: WAIT 0i8
// CHECK-NEXT-L: GOTO %STREAM_0_1
// CHECK-NEXT-L: STREAM_0_3:
// CHECK-NEXT-L: TERMINATE_THIS_SCRIPT
