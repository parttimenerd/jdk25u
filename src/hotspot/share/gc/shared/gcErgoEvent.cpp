/*
 * Copyright (c) 2026, SAP SE and/or contributors. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "gc/shared/gcErgoEvent.hpp"
#include "gc/shared/gcId.hpp"
#include "jfr/jfrEvents.hpp"
#include "runtime/os.hpp"

bool GCErgoEvent::should_emit(LogLevelType level) {
  if (level <= LogLevel::Debug) {
    return EventGCErgonomicTrace::is_enabled();
  }
  return EventGCErgonomicInfo::is_enabled();
}

void GCErgoEvent::commit(LogLevelType level, const char* tag, const char* fmt, va_list args) {
  char msg[512];
  os::vsnprintf(msg, sizeof(msg), fmt, args);

  if (level <= LogLevel::Debug) {
    EventGCErgonomicTrace event;
    if (event.should_commit()) {
      event.set_gcId(GCId::current_or_undefined());
      event.set_tag(tag);
      event.set_level(LogLevel::name(level));
      event.set_message(msg);
      event.commit();
    }
  } else {
    EventGCErgonomicInfo event;
    if (event.should_commit()) {
      event.set_gcId(GCId::current_or_undefined());
      event.set_tag(tag);
      event.set_level(LogLevel::name(level));
      event.set_message(msg);
      event.commit();
    }
  }
}
