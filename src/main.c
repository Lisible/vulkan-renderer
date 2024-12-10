#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
#define LOG(...)
#else
#define LOG(...)                                                               \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#endif

struct vulkan_renderer {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  bool enable_validation_layers;
};

#define MAX_EXTENSION_COUNT 256
#define MAX_ADDITIONAL_EXTENSION_COUNT 100
#define MAX_DEVICE_COUNT 48

VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                      VkDebugUtilsMessageTypeFlagsEXT message_type,
                      const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                      void *user_data) {
  (void)message_severity;
  (void)message_type;
  (void)user_data;
  LOG("Validation layer: %s", callback_data->pMessage);
  return VK_FALSE;
}

bool vulkan_renderer_create_instance(struct vulkan_renderer *renderer) {

  VkApplicationInfo application_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "vkguide",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "None",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0};

  const char *requested_extensions[MAX_EXTENSION_COUNT] = {0};
  uint32_t requested_extension_count = 0;
  uint32_t required_instance_extension_count;
  const char *const *required_instance_extensions =
      SDL_Vulkan_GetInstanceExtensions(&required_instance_extension_count);

  assert(requested_extension_count + required_instance_extension_count <
         MAX_EXTENSION_COUNT);
  memcpy(requested_extensions, required_instance_extensions,
         required_instance_extension_count * sizeof(const char *));
  requested_extension_count += required_instance_extension_count;

  const char *additional_extensions[MAX_ADDITIONAL_EXTENSION_COUNT] = {0};
  uint32_t additional_extension_count = 0;

  if (renderer->enable_validation_layers) {
    additional_extensions[additional_extension_count++] =
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  assert(requested_extension_count + additional_extension_count <
         MAX_EXTENSION_COUNT);
  memcpy(requested_extensions + requested_extension_count,
         additional_extensions,
         additional_extension_count * sizeof(const char *));
  requested_extension_count += additional_extension_count;

  uint32_t enabled_layer_count = 0;
  const char *const *enabled_layers = NULL;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = vulkan_debug_callback};
  if (renderer->enable_validation_layers) {
    static const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
    static const uint32_t validation_layer_count =
        sizeof(validation_layers) / sizeof(const char *);
    uint32_t available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);

    VkLayerProperties *available_layers =
        malloc(available_layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&available_layer_count,
                                       available_layers);

    bool requested_layers_found = true;
    for (uint32_t validation_layer_index = 0;
         validation_layer_index < validation_layer_count;
         validation_layer_index++) {
      bool layer_found = false;
      for (uint32_t available_layer_index = 0;
           available_layer_index < available_layer_count;
           available_layer_index++) {
        if (strcmp(validation_layers[validation_layer_index],
                   available_layers[available_layer_index].layerName) == 0) {
          layer_found = true;
          break;
        }
      }

      if (!layer_found) {
        requested_layers_found = false;
        break;
      }
    }

    free(available_layers);
    if (!requested_layers_found) {
      LOG("Not all requested layers are available.");
      goto err;
    }
    enabled_layer_count = validation_layer_count;
    enabled_layers = validation_layers;
  }

  VkInstanceCreateInfo instance_create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &application_info,
      .enabledExtensionCount = requested_extension_count,
      .ppEnabledExtensionNames = requested_extensions,
      .enabledLayerCount = enabled_layer_count,
      .ppEnabledLayerNames = enabled_layers};
  if (renderer->enable_validation_layers) {
    instance_create_info.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
  }

  VkResult create_instance_result =
      vkCreateInstance(&instance_create_info, NULL, &renderer->instance);
  if (create_instance_result != VK_SUCCESS) {
    LOG("Vulkan instance creation failed, VkResult=%d", create_instance_result);
    goto err;
  }
  return true;
err:
  return false;
}

void vulkan_renderer_destroy_instance(struct vulkan_renderer *renderer) {
  vkDestroyInstance(renderer->instance, NULL);
}
VkResult vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info,
    const VkAllocationCallbacks *allocation_callbacks,
    VkDebugUtilsMessengerEXT *debug_messenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL) {
    return func(instance, create_info, allocation_callbacks, debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, debugMessenger, pAllocator);
  }
}

bool vulkan_renderer_create_debug_messenger(struct vulkan_renderer *renderer) {
  return vkCreateDebugUtilsMessengerEXT(
             renderer->instance,
             &(const VkDebugUtilsMessengerCreateInfoEXT){
                 .sType =
                     VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                 .messageSeverity =
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                 .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                 .pfnUserCallback = vulkan_debug_callback},
             NULL, &renderer->debug_messenger) == VK_SUCCESS;
}

bool is_device_suitable(VkPhysicalDevice device) {
  (void)device;
  return true;
}

bool vulkan_renderer_pick_physical_device(struct vulkan_renderer *renderer) {
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);
  if (device_count == 0) {
    LOG("No GPU with Vulkan support found");
    goto err;
  }
  assert(device_count < MAX_DEVICE_COUNT);

  VkPhysicalDevice devices[MAX_DEVICE_COUNT];
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices);

  for (uint32_t device_index = 0; device_index < device_count; device_index++) {
    if (is_device_suitable(devices[device_count])) {
      physical_device = devices[device_count];
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    LOG("Failed to find a suitable GPU");
    goto err;
  }

  return true;
err:
  return false;
}

bool vulkan_renderer_init(struct vulkan_renderer *renderer) {
  assert(renderer);
#ifdef NDEBUG
  renderer->enable_validation_layers = false;
#else
  renderer->enable_validation_layers = true;
#endif

  if (!vulkan_renderer_create_instance(renderer)) {
    goto err;
  }

  if (renderer->enable_validation_layers) {
    if (!vulkan_renderer_create_debug_messenger(renderer)) {
      LOG("Couldn't create Vulkan renderer debug messenger.");
    }
  }

  if (!vulkan_renderer_pick_physical_device(renderer)) {
    LOG("Couldn't pick the appropriate physical device.");
    goto destroy_instance;
  }

  return true;

destroy_instance:
  if (renderer->enable_validation_layers) {
    vkDestroyDebugUtilsMessengerEXT(renderer->instance,
                                    renderer->debug_messenger, NULL);
  }
  vulkan_renderer_destroy_instance(renderer);
err:
  return false;
}

void vulkan_renderer_deinit(struct vulkan_renderer *renderer) {
  if (renderer->enable_validation_layers) {
    vkDestroyDebugUtilsMessengerEXT(renderer->instance,
                                    renderer->debug_messenger, NULL);
  }
  vulkan_renderer_destroy_instance(renderer);
}

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG("Couldn't initialize SDL: %s", SDL_GetError());
    goto err;
  }

  SDL_Window *window =
      SDL_CreateWindow("vkguide", 1280, 720, SDL_WINDOW_VULKAN);
  if (!window) {
    LOG("Couldn't create window: %s", SDL_GetError());
    goto quit_sdl;
  }

  struct vulkan_renderer renderer;
  if (!vulkan_renderer_init(&renderer)) {
    LOG("Couldn't init vulkan renderer");
    goto destroy_window;
  }

  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      bool quit = event.type == SDL_EVENT_QUIT;
      bool escape_pressed =
          event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE;

      if (quit || escape_pressed) {
        goto out_main_loop;
      }
    }

    // Render things
  }
out_main_loop:

  vulkan_renderer_deinit(&renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;

destroy_window:
  SDL_DestroyWindow(window);
quit_sdl:
  SDL_Quit();
err:
  return 1;
}
