#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR

#include "honey_vk.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <Volk/volk.h>
#include <cglm/cglm.h>

#define LOG printf("here\n")

// ========== hny_instance_t (VkInstance) ========== //
/**
 * vk_instance: the instance to wrap
 */
struct hny_instance_t {
    VkInstance vk_instance;
    struct VolkInstanceTable vk_instance_table;
};

/*
 * creates a hny_instance with validation layers & extensions. this hardcodes application &
 * engine version numbers as well as targeted vulkan api version, as well as disregards layer-specific
 * extensions, instead assuming that there are none.
 */
hny_instance_t* hny_instance_create(
    const char* const validation_layers[],
    u32 validation_layer_count,
    const char* const extensions[],
    u32 extension_count,
    const char* application_name,
    const char* engine_name
) {
    if (!application_name) return NULL;
    if (!engine_name) return NULL;

    VkResult vk_result;

    // define ptrs that will be dynamically allocated for cleanup label
    VkLayerProperties* vk_layer_properties = NULL;
    VkExtensionProperties* vk_extension_properties = NULL;

    hny_instance_t* instance = calloc(1, sizeof *instance);
    if (!instance)
        goto cleanup;

    // ====== validation layers
    if (!validation_layers || !validation_layer_count)
        goto skip_validation_layers;
    
    u32 vk_layer_property_count;
    
    vk_result = vkEnumerateInstanceLayerProperties(&vk_layer_property_count, NULL);
    if (vk_result != VK_SUCCESS)
        goto cleanup;
    vk_layer_properties = malloc(vk_layer_property_count * sizeof *vk_layer_properties);
    if (!vk_layer_properties)
        goto cleanup;
    vk_result = vkEnumerateInstanceLayerProperties(&vk_layer_property_count, vk_layer_properties);
    if (vk_result != VK_SUCCESS)
        goto cleanup;
    
    for (u32 i = 0; i < validation_layer_count; ++i) {
        for (u32 j = 0; j < vk_layer_property_count; ++j) {
            if (!strcmp(vk_layer_properties[j].layerName, validation_layers[i]))
                goto validation_layer_found;
        }
        goto cleanup;
        validation_layer_found:;
    }
    skip_validation_layers:;
    
    // ===== extensions
    if (!extensions || !extension_count)
        goto skip_extensions;
    
    u32 vk_extension_property_count;

    // hardcoded: we assume non-layer based extenensions w/ first NULL
    vk_result = vkEnumerateInstanceExtensionProperties(NULL, &vk_extension_property_count, NULL);
    if (vk_result != VK_SUCCESS)
        goto cleanup;
    vk_extension_properties = malloc(vk_extension_property_count * sizeof *vk_extension_properties);
    if (!vk_extension_properties)
        goto cleanup;
    vk_result = vkEnumerateInstanceExtensionProperties(NULL, &vk_extension_property_count, vk_extension_properties);
    if (vk_result != VK_SUCCESS)
        goto cleanup;

    for (u32 i = 0; i < extension_count; ++i) {
        for (u32 j = 0; j < vk_extension_property_count; ++j) {
            if (!strcmp(vk_extension_properties[j].extensionName, extensions[i]))
                goto extension_found;
        }
        goto cleanup;
        extension_found:;
    }
    skip_extensions:;
    
    // ===== application info
    // hardcoded: target vulkan 1.3, disregard application & engine version
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = engine_name,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    // ===== instance create info
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = validation_layer_count,
        .ppEnabledLayerNames = validation_layers,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions
    };
    
    vk_result = vkCreateInstance(&create_info, NULL, &instance->vk_instance);
    if (vk_result != VK_SUCCESS)
        goto cleanup;

    free(vk_layer_properties);
    free(vk_extension_properties);
    volkLoadInstanceTable(&instance->vk_instance_table, instance->vk_instance);
    return instance;

    cleanup:
        free(vk_layer_properties);
        free(vk_extension_properties);
        free(instance);
        return NULL;
}

/*
 * destroys the hny_instance_t 
 */
void hny_instance_destroy(hny_instance_t* instance) {
    if (instance) {
        instance->vk_instance_table.vkDestroyInstance(instance->vk_instance, NULL);
        free(instance);
    }
}

// ========== hny_debug_utils_messenger_callback_fn_t (PFN_vkDebugUtilsMessengerCallbackEXT) ========== //
typedef struct hny_debug_utils_messenger_callback_wrapper_t {
    hny_debug_utils_messenger_callback_fn_t debug_utils_messenger_callback;
} hny_debug_utils_messenger_callback_wrapper_t;

/**
 * currently only gives bacak the callback data's message
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT vk_severity,
    VkDebugUtilsMessageTypeFlagsEXT vk_type,
    const VkDebugUtilsMessengerCallbackDataEXT* vk_pCallbackData,
    void* vk_pUserData
) {
    hny_debug_utils_message_severity_t severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_SENTINEL;
    hny_debug_utils_message_type_flags_t type = HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_SENTINEL;
    const char* message = NULL;

    switch (vk_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
            severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE;
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_INFO;
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING;
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR;
            break;
        }
        default: {
            severity = HNY_DEBUG_UTILS_MESSAGE_SEVERITY_SENTINEL;
            break;
        }
    }

    if (vk_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        type |= HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_GENERAL;
    if (vk_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        type |= HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_VALIDATION;
    if (vk_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        type |= HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_PERFORMANCE;
    if (vk_type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
        type |= HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_DEVICE_ADDRESS_BINDING;

    message = vk_pCallbackData->pMessage;

    if (vk_pUserData)
        ((hny_debug_utils_messenger_callback_wrapper_t*) vk_pUserData)->debug_utils_messenger_callback(severity, type, message);
    // don't abort the operation that triggered the callback
    return VK_FALSE;
}

// ========== hny_debug_utils_messenger_t (VkDebugUtilsMessengerEXT) ========== //
/*
 * vk_instance: the instance the vk_debug... is tied to (max 1)
 * vk_debug...: the actual messenger (instances can have multiple) 
 */
struct hny_debug_utils_messenger_t {
    hny_instance_t* instance;
    hny_debug_utils_messenger_callback_wrapper_t debug_utils_messenger_callback_wrapper;
    VkDebugUtilsMessengerEXT vk_debug_utils_messenger;
};

/*
 * creates a hny_debug_utils_messenger_t with:
 * severity flags: warning | error
 * message type flags: general | performance | validation
 */
hny_debug_utils_messenger_t* hny_debug_utils_messenger_create(
    hny_instance_t* instance,
    hny_debug_utils_messenger_callback_fn_t debug_utils_messenger_callback
) {
    if (!instance)
        return NULL;
    if (!debug_utils_messenger_callback)
        return NULL;
    
    VkResult vk_result;

    hny_debug_utils_messenger_t* debug_utils_messenger = calloc(1, sizeof *debug_utils_messenger);
    if (!debug_utils_messenger)
        return NULL;

    debug_utils_messenger->debug_utils_messenger_callback_wrapper.debug_utils_messenger_callback = debug_utils_messenger_callback;

    // hardcoded
    VkDebugUtilsMessageSeverityFlagsEXT severity_flags = (
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
    );

    // hardcoded
    VkDebugUtilsMessageTypeFlagsEXT message_type_flags = (
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
    );
    
    // dynamic for severity_flags & message_type_flags
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severity_flags,
        .messageType = message_type_flags,
        .pfnUserCallback = debug_callback,
        .pUserData = &debug_utils_messenger->debug_utils_messenger_callback_wrapper
    };
    
    vk_result = instance->vk_instance_table.vkCreateDebugUtilsMessengerEXT(
        instance->vk_instance,
        &debug_create_info,
        NULL,
        &debug_utils_messenger->vk_debug_utils_messenger
    );
    if (vk_result != VK_SUCCESS)
        return free(debug_utils_messenger), NULL;

    debug_utils_messenger->instance = instance;
    return debug_utils_messenger;
}

/*
 * destroys the hny_debug_utils_messenger_t
 */
void hny_debug_utils_messenger_destroy(hny_debug_utils_messenger_t* debug_utils_messenger) {
    if (debug_utils_messenger) {
        debug_utils_messenger->instance->vk_instance_table.vkDestroyDebugUtilsMessengerEXT(
            debug_utils_messenger->instance->vk_instance,
            debug_utils_messenger->vk_debug_utils_messenger,
            NULL
        );
        free(debug_utils_messenger);
    }
}

// ========== hny_physical_device_t (VkPhysicalDevice) ========== //
struct hny_physical_device_t {
    hny_instance_t* instance;
    VkPhysicalDevice vk_physical_device;
};

/*
 * currently, this function scores each found physical device on a set criteria that is pretty
 * limited. this should change. minimum support vulkan 1.3, and required device extensions are
 * decied by honey (only VK_KHR_SWAPCHAIN_EXTENSION_NAME)
 */
hny_physical_device_t* hny_physical_device_create(hny_instance_t* instance) {
    if (!instance)
        return NULL;

    VkResult vk_result;

    // define ptrs that will be dynamically allocated for cleanup label
    VkPhysicalDevice* vk_physical_devices = NULL;

    hny_physical_device_t* physical_device = calloc(1, sizeof *physical_device);
    if (!physical_device)
        goto cleanup;

    // ===== physical devices
    u32 vk_physical_device_count;
    
    vk_result = instance->vk_instance_table.vkEnumeratePhysicalDevices(instance->vk_instance, &vk_physical_device_count, NULL);
    if (vk_result != VK_SUCCESS)
        goto cleanup;
    if (!vk_physical_device_count)
        goto cleanup;
    vk_physical_devices = malloc(vk_physical_device_count * sizeof *vk_physical_devices);
    if (!vk_physical_devices)
        goto cleanup;
    vk_result = instance->vk_instance_table.vkEnumeratePhysicalDevices(
        instance->vk_instance,
        &vk_physical_device_count,
        vk_physical_devices
    );
    if (vk_result != VK_SUCCESS)
        goto cleanup;
    
    // hardcoded
    const char* const required_device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    u32 required_device_extension_count = sizeof required_device_extensions / sizeof *required_device_extensions;

    // hardcoded
    VkFlags required_queue_family_flags = (
        VK_QUEUE_GRAPHICS_BIT
    );
    
    u32 best_physical_device_score = 0, best_physical_device_index = 0;
    for (u32 i = 0; i < vk_physical_device_count; ++i) {
        VkPhysicalDeviceProperties2 physical_device_properties = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
        };
        instance->vk_instance_table.vkGetPhysicalDeviceProperties2(vk_physical_devices[i], &physical_device_properties);
        
        // ===== filters
        VkExtensionProperties* vk_device_extension_properties = NULL;
        VkQueueFamilyProperties2* vk_device_queue_family_properties = NULL;
        
        // api version compatibility check
        if (physical_device_properties.properties.apiVersion < VK_API_VERSION_1_3)
            goto skip_physical_device_scoring;

        // device extension requirements
        u32 vk_device_extension_property_count;
        vk_result = instance->vk_instance_table.vkEnumerateDeviceExtensionProperties(
            vk_physical_devices[i],
            NULL,
            &vk_device_extension_property_count,
            NULL
        );
        if (vk_result != VK_SUCCESS)
            goto skip_physical_device_scoring;
        vk_device_extension_properties = malloc(vk_device_extension_property_count * sizeof *vk_device_extension_properties);
        if (!vk_device_extension_properties)
            goto skip_physical_device_scoring;
        vk_result = instance->vk_instance_table.vkEnumerateDeviceExtensionProperties(
            vk_physical_devices[i],
            NULL,
            &vk_device_extension_property_count,
            vk_device_extension_properties
        );
        if (vk_result != VK_SUCCESS)
            goto skip_physical_device_scoring;
        
        for (u32 j = 0; j < required_device_extension_count; ++j) {
            for (u32 k = 0; k < vk_device_extension_property_count; ++k) {
                if (!strcmp(vk_device_extension_properties[k].extensionName, required_device_extensions[j]))
                    goto extension_found;
            }
            goto skip_physical_device_scoring;
            extension_found:;
        }
        
        // queue family check
        u32 vk_device_queue_family_property_count;

        instance->vk_instance_table.vkGetPhysicalDeviceQueueFamilyProperties2(
            vk_physical_devices[i],
            &vk_device_queue_family_property_count,
            NULL
        );
        vk_device_queue_family_properties = malloc(vk_device_queue_family_property_count * sizeof *vk_device_queue_family_properties);
        if (!vk_device_queue_family_properties)
            goto skip_physical_device_scoring;
        for (u32 j = 0; j < vk_device_queue_family_property_count; ++j)
            vk_device_queue_family_properties[j] = (VkQueueFamilyProperties2) {
                .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
                .pNext = NULL
            };
        instance->vk_instance_table.vkGetPhysicalDeviceQueueFamilyProperties2(
            vk_physical_devices[i],
            &vk_device_queue_family_property_count,
            vk_device_queue_family_properties
        );
        
        for (u32 j = 0; j < vk_device_queue_family_property_count; ++j) {
            if (
                (vk_device_queue_family_properties[j].queueFamilyProperties.queueFlags & required_queue_family_flags) ==
                required_queue_family_flags
            )
                goto queue_flag_found;
        }
        goto skip_physical_device_scoring;
        queue_flag_found:;

        // device features check
        VkPhysicalDeviceVulkan11Features required_device_features_v11 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = NULL
        };

        VkPhysicalDeviceVulkan13Features required_device_features_v13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &required_device_features_v11
        };

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT required_device_features_eds = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
            .pNext = &required_device_features_v13
        };

        VkPhysicalDeviceFeatures2 required_device_features_2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &required_device_features_eds
        };

        instance->vk_instance_table.vkGetPhysicalDeviceFeatures2(vk_physical_devices[i], &required_device_features_2);
        if (
            !required_device_features_v11.shaderDrawParameters ||
            !required_device_features_v13.dynamicRendering ||
            !required_device_features_eds.extendedDynamicState
        )
            goto skip_physical_device_scoring;

        u32 physical_device_score = 0;
        switch (physical_device_properties.properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
                physical_device_score += 1000;
                break;
            }
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
                physical_device_score += 500;
                break;
            }
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
                physical_device_score += 250;
                break;
            }
            case VK_PHYSICAL_DEVICE_TYPE_CPU: {
                physical_device_score += 100;
                break;
            }
            default:
                physical_device_score += 1;
                break;
        }

        if (physical_device_score > best_physical_device_score) {
            best_physical_device_score = physical_device_score;
            best_physical_device_index = i;
        }

        skip_physical_device_scoring:
            free(vk_device_extension_properties);
            free(vk_device_queue_family_properties);
    }
    
    if (!best_physical_device_score)
        goto cleanup;

    *physical_device = (hny_physical_device_t) {
        .instance = instance,
        .vk_physical_device = vk_physical_devices[best_physical_device_index]
    };

    free(vk_physical_devices);
    return (physical_device);

    cleanup:
        free(physical_device);
        free(vk_physical_devices);
        return NULL;
}

void hny_physical_device_destroy(hny_physical_device_t* physical_device) {
    free(physical_device);
}

bool hny_vk_init(void) {
    return volkInitialize() == VK_SUCCESS;
}