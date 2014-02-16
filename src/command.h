#ifndef _Z4CTRL_COMMAND_H_
#define _Z4CTRL_COMMAND_H_

#define READ_POWER_STATUS       "CR0\r"
#define READ_INPUT_MODE         "CR1\r"
#define READ_LAMP_HOURS         "CR3\r"
#define READ_MODEL_NUMBER       "CR5\r"
#define READ_TEMP_SENSORS       "CR6\r"

#define POWER_ON                "C00\r"
#define POWER_OFF_QUICK         "C01\r"
#define POWER_OFF_ASK           "C02\r"

#define MUTE_ON                 "C0D\r"
#define MUTE_OFF                "C0E\r"

#define SCALE_NORMAL            "C0F\r"
#define SCALE_FULL              "C10\r"
#define SCALE_ZOOM              "C2C\r"
#define SCALE_WIDE_1            "C2D\r"
#define SCALE_WIDE_2            "C2E\r"
#define SCALE_CAPTION           "C63\r"
#define SCALE_FULL_THROUGH      "C65\r"
#define SCALE_NORMAL_THROUGH    "C66\r"

#define LAMP_AUTO_1             "C72\r"
#define LAMP_AUTO_2             "C73\r"
#define LAMP_NORMAL             "C74\r"
#define LAMP_ECONOMY            "C75\r"

#define MENU_ON                 "C1C\r"
#define MENU_OFF                "C1D\r"
#define MENU_CLEAR              "C1E\r"

#define INPUT_COMPOSIT          "C23\r"
#define INPUT_SVIDEO            "C24\r"
#define INPUT_COMPONENT_1       "C25\r"
#define INPUT_COMPONENT_2       "C26\r"
#define INPUT_VGA               "C50\r"
#define INPUT_HDMI              "C53\r"

#define PRESS_RIGHT             "C3A\r"
#define PRESS_LEFT              "C3B\r"
#define PRESS_UP                "C3C\r"
#define PRESS_DOWN              "C3D\r"
#define PRESS_ENTER             "C3F\r"

#define COLOR_LIVING            "C11\r"
#define COLOR_CREATIVE          "C12\r"
#define COLOR_CINEMA            "C13\r"
#define COLOR_USER_1            "C14\r"
#define COLOR_USER_2            "C15\r"
#define COLOR_USER_3            "C16\r"
#define COLOR_USER_4            "C17\r"
#define COLOR_VIVID             "C18\r"
#define COLOR_DYNAMIC           "C19\r"
#define COLOR_POWERFUL          "C1A\r"
#define COLOR_NATURAL           "C1B\r"

#define LOGO_OFF                "C7A\r"
#define LOGO_DEFAULT            "C7B\r"
#define LOGO_USER               "C7C\r"
#define LOGO_CAPTURE            "C7D\r"

#define POWER_MANAGEMENT_ON     "C2A\r"
#define POWER_MANAGEMENT_OFF    "C2B\r"

#define D4_CONTROL_ON           "C67\r"
#define D4_CONTROL_OFF          "C68\r"

#define CEILING_ON              "C76\r"
#define CEILING_OFF             "C77\r"
#define REAR_ON                 "C78\r"
#define REAR_OFF                "C79\r"

#define PC_ADJUST               "C89\r"

#define KEYSTONE_PLUS           "C8E\r"
#define KEYSTONE_MINUS          "C8F\r"

#endif // _Z4CTRL_COMMAND_H_
