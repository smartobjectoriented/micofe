# Copyright (c) 2025-2026 EDGEMTech SA
# Adapted for MICOFE - Copyright (c) 2026 REDS Institute, HEIG-VD

require bsp_arm_common.inc

def __do_platform_deploy(d):

        print("Platorm " + d.getVar('IB_PLATFORM'))

        __deploy_arm_common(d)


