/**
 * collectd - src/unload_test_a.c
 *
 * Minimal removable plugin used by src/unload_test.sh. Not installed.
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

static gauge_t reads;

static int unload_test_a_read(void) {
  reads++;

  value_list_t vl = VALUE_LIST_INIT;
  vl.values = &(value_t){.gauge = reads};
  vl.values_len = 1;
  sstrncpy(vl.plugin, "unload_test_a", sizeof(vl.plugin));
  sstrncpy(vl.type, "gauge", sizeof(vl.type));
  return plugin_dispatch_values(&vl);
}

static int unload_test_a_shutdown(void) {
  INFO("unload_test_a plugin: shutdown after %.0f reads.", reads);
  return 0;
}

void module_register(void) {
  plugin_register_read("unload_test_a", unload_test_a_read);
  plugin_register_shutdown("unload_test_a", unload_test_a_shutdown);
}

int module_unregister(void) {
  reads = 0;
  return 0;
}
