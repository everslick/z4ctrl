#ifndef _Z4CTRL_COMMAND_H_
#define _Z4CTRL_COMMAND_H_

#define READ_POWER_STATUS       "CR0\r"
#define READ_INPUT_MODE         "CR1\r"
#define READ_LAMP_HOURS         "CR3\r"
#define READ_MODEL_NUMBER       "CR5\r"
#define READ_TEMP_SENSORS       "CR6\r"

#define POWER_ON                "C00\r"
#define POWER_OFF               "C01\r"

#define SCALE_NORMAL            "C0D\r"
#define SCALE_FULL              "C0E\r"
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

#define INPUT_COMPOSIT          "C23\r"
#define INPUT_SVIDEO            "C24\r"
#define INPUT_COMPONENT_1       "C25\r"
#define INPUT_COMPONENT_2       "C26\r"
#define INPUT_VGA               "C50\r"
#define INPUT_HDMI              "C53\r"

#define COLOR_LIVING            "C39\r"
#define COLOR_CREATIVE          "C3A\r"
#define COLOR_CINEMA            "C3B\r"
#define COLOR_USER_1            "C3C\r"
#define COLOR_USER_2            "C3D\r"
#define COLOR_USER_3            "C3E\r"
#define COLOR_USER_4            "C3F\r"
#define COLOR_VIVID             "C40\r"
#define COLOR_DYNAMIC           "C41\r"
#define COLOR_POWERFUL          "C42\r"
#define COLOR_NATURAL           "C43\r"

#endif // _Z4CTRL_COMMAND_H_
