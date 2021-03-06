/*
 * This file is part of linux-driver-management.
 *
 * Copyright © 2016-2018 Linux Driver Management Developers, Solus Project
 *
 * linux-driver-management is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <umockdev.h>

#include "ldm-private.h"
#include "ldm.h"
#include "util.h"

DEF_AUTOFREE(UMockdevTestbed, g_object_unref)

#define NV_MOCKDEV_FILE TEST_DATA_ROOT "/nvidia1060.umockdev"
#define OPTIMUS_MOCKDEV_FILE TEST_DATA_ROOT "/optimus765m.umockdev"
#define BLUETOOTH_UMOCKDEV_FILE TEST_DATA_ROOT "/bluetoothUSB.umockdev"
#define WIFI_UMOCKDEV_FILE TEST_DATA_ROOT "/wifi.umockdev"

START_TEST(test_manager_simple)
{
        g_autoptr(LdmManager) manager = NULL;
        autofree(UMockdevTestbed) *bed = NULL;
        g_autoptr(GPtrArray) devices = NULL;
        LdmDevice *nvidia_device = NULL;
        gint vendor_id = 0;

        bed = umockdev_testbed_new();
        fail_if(!umockdev_testbed_add_from_file(bed, NV_MOCKDEV_FILE, NULL),
                "Failed to create NVIDIA device");
        manager = ldm_manager_new(0);
        fail_if(!manager, "Failed to get the LdmManager");

        devices = ldm_manager_get_devices(manager, LDM_DEVICE_TYPE_GPU);
        fail_if(!devices, "Failed to obtain devices");
        fail_if(devices->len != 1, "Invalid device set");

        /* Grab the NVIDIA device */
        nvidia_device = devices->pdata[0];
        fail_if(!ldm_device_has_type(nvidia_device, LDM_DEVICE_TYPE_PCI),
                "PCI GPU isn't classified as PCI!");

        fail_if(!ldm_device_has_type(nvidia_device, LDM_DEVICE_TYPE_GPU),
                "PCI GPU isn't classified as GPU!");

        fail_if(!ldm_device_has_attribute(nvidia_device, LDM_DEVICE_ATTRIBUTE_BOOT_VGA),
                "PCI GPU lacks boot_vga attribute");

        vendor_id = ldm_device_get_vendor_id(nvidia_device);
        fail_if(vendor_id != LDM_PCI_VENDOR_ID_NVIDIA, "NVIDIA device vendor is not NVIDIA");
}
END_TEST

/**
 * Much like the simple test but will ensure we actually find the GPU parts
 * for an optimus system.
 */
START_TEST(test_manager_optimus)
{
        g_autoptr(LdmManager) manager = NULL;
        autofree(UMockdevTestbed) *bed = NULL;
        g_autoptr(GPtrArray) devices = NULL;
        LdmDevice *igpu = NULL;
        LdmDevice *dgpu = NULL;
        gint vendor_id = 0;

        bed = umockdev_testbed_new();
        fail_if(!umockdev_testbed_add_from_file(bed, OPTIMUS_MOCKDEV_FILE, NULL),
                "Failed to create Optimus device");
        manager = ldm_manager_new(0);
        fail_if(!manager, "Failed to get the LdmManager");

        devices = ldm_manager_get_devices(manager, LDM_DEVICE_TYPE_GPU);
        fail_if(!devices, "Failed to obtain devices");
        fail_if(devices->len != 2, "Invalid device set");

        /* Check the dGPU data is correct */
        dgpu = devices->pdata[1];
        vendor_id = ldm_device_get_vendor_id(dgpu);
        fail_if(vendor_id != LDM_PCI_VENDOR_ID_NVIDIA, "dGPU Vendor is not NVIDIA");

        /* Check the iGPU data is correct */
        igpu = devices->pdata[0];
        vendor_id = ldm_device_get_vendor_id(igpu);
        fail_if(vendor_id != LDM_PCI_VENDOR_ID_INTEL, "iGPU vendor is not Intel");

        fail_if(!ldm_device_has_attribute(igpu, LDM_DEVICE_ATTRIBUTE_BOOT_VGA),
                "iGPU has missing boot_vga attribute");

        /* Does iGPU have PCI/GPU? */
        fail_if(!ldm_device_has_type(igpu, LDM_DEVICE_TYPE_PCI | LDM_DEVICE_TYPE_GPU),
                "iGPU has missing PCI/GPU classification");

        /* Does dGPU have PCI/GPU? */
        fail_if(!ldm_device_has_type(dgpu, LDM_DEVICE_TYPE_PCI | LDM_DEVICE_TYPE_GPU),
                "dGPU has missing PCI/GPU classification");

        fail_if(ldm_device_has_attribute(dgpu, LDM_DEVICE_ATTRIBUTE_BOOT_VGA),
                "dGPU should not have boot_vga attribute");
}
END_TEST

/**
 * Find a bluetooth controller connected via USB (can be internal)
 */
START_TEST(test_manager_bluetooth_usb)
{
        g_autoptr(LdmManager) manager = NULL;
        autofree(UMockdevTestbed) *bed = NULL;
        g_autoptr(GPtrArray) devices = NULL;
        LdmDevice *device = NULL;

        bed = umockdev_testbed_new();
        fail_if(!umockdev_testbed_add_from_file(bed, BLUETOOTH_UMOCKDEV_FILE, NULL),
                "Failed to create Bluetooth device");
        manager = ldm_manager_new(LDM_MANAGER_FLAGS_NO_MONITOR);
        fail_if(!manager, "Failed to get the LdmManager");

        devices = ldm_manager_get_devices(manager, LDM_DEVICE_TYPE_BLUETOOTH);
        fail_if(!devices, "Failed to obtain devices");
        fail_if(devices->len != 1, "Invalid device set");

        device = devices->pdata[0];
        fail_if(!ldm_device_has_type(device, LDM_DEVICE_TYPE_USB),
                "Device should be identified as USB!");
        fail_if(!ldm_device_has_attribute(device, LDM_DEVICE_ATTRIBUTE_HOST),
                "Bluetooth device not marked as a host controller");
}
END_TEST

START_TEST(test_manager_wifi_pci)
{
        g_autoptr(LdmManager) manager = NULL;
        autofree(UMockdevTestbed) *bed = NULL;
        g_autoptr(GPtrArray) devices = NULL;
        LdmDevice *device = NULL;

        bed = umockdev_testbed_new();
        fail_if(!umockdev_testbed_add_from_file(bed, WIFI_UMOCKDEV_FILE, NULL),
                "Failed to create WiFI device");
        manager = ldm_manager_new(LDM_MANAGER_FLAGS_NO_MONITOR);
        fail_if(!manager, "Failed to get the LdmManager");

        devices = ldm_manager_get_devices(manager, LDM_DEVICE_TYPE_WIRELESS);
        fail_if(!devices, "Failed to obtain devices");
        fail_if(devices->len != 1, "Invalid device set");

        device = devices->pdata[0];
        fail_if(!ldm_device_has_type(device, LDM_DEVICE_TYPE_PCI), "Device has wrong type");
}
END_TEST

/**
 * Standard helper for running a test suite
 */
static int ldm_test_run(Suite *suite)
{
        SRunner *runner = NULL;
        int n_failed = 0;

        runner = srunner_create(suite);
        srunner_run_all(runner, CK_VERBOSE);
        n_failed = srunner_ntests_failed(runner);
        srunner_free(runner);

        return n_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static Suite *test_create(void)
{
        Suite *s = NULL;
        TCase *tc = NULL;

        s = suite_create(__FILE__);
        tc = tcase_create(__FILE__);
        suite_add_tcase(s, tc);

        tcase_add_test(tc, test_manager_simple);
        tcase_add_test(tc, test_manager_optimus);
        tcase_add_test(tc, test_manager_bluetooth_usb);
        tcase_add_test(tc, test_manager_wifi_pci);

        return s;
}

int main(__ldm_unused__ int argc, __ldm_unused__ char **argv)
{
        return ldm_test_run(test_create());
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
