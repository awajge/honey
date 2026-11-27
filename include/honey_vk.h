#ifndef HONEY_VK_H
#define HONEY_VK_H

#include <stdbool.h>
#include <stdint.h>

#define HNY_VK_LAYER_KHRONOS_VALIDATION "VK_LAYER_KHRONOS_validation"

#define HNY_VK_EXT_DEBUG_UTILS      "VK_EXT_debug_utils"
#define HNY_VK_KHR_WIN32_SURFACE    "VK_KHR_win32_surface"
#define HNY_VK_KHR_SURFACE          "VK_KHR_surface"

typedef struct hny_instance_t hny_instance_t;
typedef struct hny_debug_utils_messenger_t hny_debug_utils_messenger_t;
typedef struct hny_physical_device_t hny_physical_device_t;

typedef enum hny_debug_utils_message_severity_t {
    HNY_DEBUG_UTILS_MESSAGE_SEVERITY_SENTINEL,
    HNY_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE,
    HNY_DEBUG_UTILS_MESSAGE_SEVERITY_INFO,
    HNY_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING,
    HNY_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR
} hny_debug_utils_message_severity_t;

typedef enum hny_debug_utils_message_type_flag_bit_t {
    HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_SENTINEL = 0u,
    HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_GENERAL = 1u << 0,
    HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_VALIDATION = 1u << 1,
    HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_PERFORMANCE = 1u << 2,
    HNY_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BIT_DEVICE_ADDRESS_BINDING = 1u << 3,
} hny_debug_utils_message_type_flag_bit_t;
typedef uint32_t hny_debug_utils_message_type_flags_t;

typedef void (*hny_debug_utils_messenger_callback_fn_t)(
    hny_debug_utils_message_severity_t,
    hny_debug_utils_message_type_flags_t,
    const char*
);

hny_instance_t* hny_instance_create(
    const char* const validation_layers[],
    uint32_t validation_layer_count,
    const char* const extensions[],
    uint32_t extension_count,
    const char* application_name,
    const char* engine_name
);
hny_debug_utils_messenger_t* hny_debug_utils_messenger_create(
    hny_instance_t* instance,
    hny_debug_utils_messenger_callback_fn_t debug_utils_messenger_callback
);
hny_physical_device_t* hny_physical_device_create(hny_instance_t* instance);

void hny_instance_destroy(hny_instance_t* instance);
void hny_debug_utils_messenger_destroy(hny_debug_utils_messenger_t* debug_utils_messenger);
void hny_physical_device_destroy(hny_physical_device_t* physical_device);

bool hny_vk_init(void);

#endif