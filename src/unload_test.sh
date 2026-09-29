#!/bin/sh
#
# collectd - src/unload_test.sh
#
# End-to-end test of RECONFIGURE's LoadPlugin/UnloadPlugin against a running
# daemon, using the removable test plugins unload_test_a and unload_test_b.
# Must be run from the build directory; exits 77 (skip) if the unixsock or
# logfile plugins weren't built.

set -u

BUILDDIR="$(pwd)"
SRCDIR="${srcdir:-$BUILDDIR}"
PLUGINDIR="$BUILDDIR/.libs"

for p in unixsock logfile unload_test_a unload_test_b; do
  if [ ! -f "$PLUGINDIR/$p.so" ]; then
    echo "SKIP: $PLUGINDIR/$p.so not built"
    exit 77
  fi
done

TMP="$(mktemp -d "${TMPDIR:-/tmp}/collectd-unload-test.XXXXXX")"
SOCK="$TMP/sock"
LOG="$TMP/collectd.log"
PID=""

cleanup() {
  if [ -n "$PID" ]; then
    kill "$PID" 2>/dev/null
    wait "$PID" 2>/dev/null
  fi
  rm -rf "$TMP"
}
trap cleanup EXIT

fail() {
  echo "FAIL: $*"
  echo "--- collectd log ---"
  cat "$LOG" 2>/dev/null
  exit 1
}

ctl() {
  "$BUILDDIR/collectdctl" -s "$SOCK" "$@"
}

# Polls until $1 (a shell condition) succeeds, for at most 10 seconds.
wait_for() {
  i=0
  while [ $i -lt 50 ]; do
    if eval "$1"; then
      return 0
    fi
    sleep 0.2
    i=$((i + 1))
  done
  return 1
}

value_of() {
  ctl getval "test-host/$1/gauge" 2>/dev/null | sed -n 's/^value=//p'
}

reconfigure() {
  printf '%s\n' "$1" >"$TMP/reconfig.conf"
  ctl reconfigure "$TMP/reconfig.conf"
}

cat >"$TMP/collectd.conf" <<EOF
Hostname "test-host"
BaseDir "$TMP"
PIDFile "$TMP/collectd.pid"
PluginDir "$PLUGINDIR"
TypesDB "$SRCDIR/src/types.db"
Interval 1

LoadPlugin logfile
<Plugin logfile>
  LogLevel "info"
  File "$LOG"
</Plugin>

LoadPlugin unixsock
<Plugin unixsock>
  SocketFile "$SOCK"
</Plugin>
EOF

"$BUILDDIR/collectd" -f -C "$TMP/collectd.conf" &
PID=$!

wait_for "[ -S '$SOCK' ]" || fail "daemon did not open its socket"

# Add both plugins.
out="$(reconfigure '<LoadPlugin unload_test_a>
</LoadPlugin>
<LoadPlugin unload_test_b>
</LoadPlugin>')"
echo "$out" | grep -q "2 plugin(s) added" || fail "add: unexpected reply: $out"

wait_for '[ -n "$(value_of unload_test_a)" ]' || fail "unload_test_a never reported"
wait_for '[ -n "$(value_of unload_test_b)" ]' || fail "unload_test_b never reported"
wait_for '[ "$(value_of unload_test_a | cut -d. -f1)" -ge 3 ]' ||
  fail "unload_test_a did not reach 3 reads"

# Remove both. unload_test_b sleeps in its read callback, so this usually
# exercises waiting for an in-flight read.
out="$(reconfigure '<UnloadPlugin unload_test_a>
</UnloadPlugin>
<UnloadPlugin unload_test_b>
</UnloadPlugin>')"
echo "$out" | grep -q "2 plugin(s) removed" || fail "remove: unexpected reply: $out"

grep -q "unload_test_a plugin: shutdown" "$LOG" || fail "A's shutdown callback did not run"
grep -q "unload_test_b plugin: shutdown" "$LOG" || fail "B's shutdown callback did not run"
grep -q "unload_test_b plugin: freeing state" "$LOG" || fail "B's user_data was not freed"
grep -q 'plugin "unload_test_a" successfully unloaded' "$LOG" || fail "A was not dlclose()'d"
grep -q 'plugin "unload_test_b" successfully unloaded' "$LOG" || fail "B was not dlclose()'d"

# Re-add A; it must start counting from scratch (it was >= 3 before).
out="$(reconfigure '<LoadPlugin unload_test_a>
</LoadPlugin>')"
echo "$out" | grep -q "1 plugin(s) added" || fail "re-add: unexpected reply: $out"
wait_for '[ "$(value_of unload_test_a | cut -d. -f1)" -le 2 ]' ||
  fail "re-added unload_test_a did not restart counting"

# Plugins without module_unregister() must be refused.
out="$(reconfigure '<UnloadPlugin logfile>
</UnloadPlugin>')"
echo "$out" | grep -q "0 plugin(s) removed" || fail "non-removable: unexpected reply: $out"
grep -q "Plugin \`logfile' does not support removal" "$LOG" ||
  fail "non-removable plugin was not refused"

kill -0 "$PID" 2>/dev/null || fail "daemon died"

echo "PASS"
exit 0
