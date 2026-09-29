/**
 * collectd - src/unload_test_b.c
 *
 * Removable plugin used by src/unload_test.sh. Not installed. Unlike
 * unload_test_a it uses an init callback, a complex read whose user_data
 * free function lives in this plugin, and a slow read so an unload request
 * is likely to arrive while the read callback is executing.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **/

#include "collectd.h"

#include "plugin.h"
#include "utils/common/common.h"

typedef struct {
  gauge_t reads;
} unload_test_b_state_t;

static void unload_test_b_free(void *data) {
  INFO("unload_test_b plugin: freeing state.");
  free(data);
}

static int unload_test_b_read(user_data_t *ud) {
  unload_test_b_state_t *state = ud->data;

  usleep(300000);
  state->reads++;

  value_list_t vl = VALUE_LIST_INIT;
  vl.values = &(value_t){.gauge = state->reads};
  vl.values_len = 1;
  sstrncpy(vl.plugin, "unload_test_b", sizeof(vl.plugin));
  sstrncpy(vl.type, "gauge", sizeof(vl.type));
  return plugin_dispatch_values(&vl);
}

static int unload_test_b_init(void) {
  unload_test_b_state_t *state = calloc(1, sizeof(*state));
  if (state == NULL)
    return ENOMEM;

  return plugin_register_complex_read(
      /* group = */ NULL, "unload_test_b", unload_test_b_read,
      /* interval = */ 0,
      &(user_data_t){.data = state, .free_func = unload_test_b_free});
}

static int unload_test_b_shutdown(void) {
  INFO("unload_test_b plugin: shutdown.");
  return 0;
}

void module_register(void) {
  plugin_register_init("unload_test_b", unload_test_b_init);
  plugin_register_shutdown("unload_test_b", unload_test_b_shutdown);
}

int module_unregister(void) { return 0; }
