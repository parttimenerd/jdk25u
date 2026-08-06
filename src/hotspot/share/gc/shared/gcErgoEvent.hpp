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

#ifndef SHARE_GC_SHARED_GCERGOEVENT_HPP
#define SHARE_GC_SHARED_GCERGOEVENT_HPP

#include "logging/logLevel.hpp"
#include "logging/logTag.hpp"
#include "logging/logTagSet.hpp"
#include "memory/allocation.hpp"
#include "utilities/globalDefinitions.hpp"

#include <stdarg.h>

// Emits the gc+ergo narration as the GCErgonomicInfo / GCErgonomicTrace JFR
// events, independent of any -Xlog configuration.
//
// The stock log_info/log_debug/log_trace macros short-circuit to (void)0 when
// no -Xlog output is attached (the printf argument list is never evaluated), so
// a hook inside the logging pipeline can only capture gc+ergo lines when -Xlog
// is already enabled. To make the JFR events self-sufficient, the log_ergo
// wrapper macro expands to a GCErgoNarration functor whose operator() formats
// the message once and (a) commits the JFR event when it is enabled and (b)
// writes the text line when -Xlog is enabled.

class GCErgoEvent : public AllStatic {
public:
  // True when GCErgonomicInfo (level >= Info) or GCErgonomicTrace (level <=
  // Debug) is enabled. Lets callers behind a log_is_enabled(gc, ergo) guard
  // also run when only the JFR event -- not -Xlog -- is enabled.
  static bool should_emit(LogLevelType level);

  // Formats fmt/args and commits GCErgonomicTrace (level <= Debug) or
  // GCErgonomicInfo (otherwise), independent of -Xlog. tag is the "+"-joined
  // tag set, e.g. "gc+ergo" or "gc+ergo+cset".
  static void commit(LogLevelType level, const char* tag, const char* fmt, va_list args);
};

// Functor templated on the tag set (mirrors LogImpl<LOG_TAGS(...)>). Carries
// the level and the target LogTagSet; operator() takes the printf args, exactly
// like the value returned by the stock log_* macros.
template <LogTagType T0, LogTagType T1 = LogTag::__NO_TAG, LogTagType T2 = LogTag::__NO_TAG,
          LogTagType T3 = LogTag::__NO_TAG, LogTagType T4 = LogTag::__NO_TAG,
          LogTagType GuardTag = LogTag::__NO_TAG>
class GCErgoNarration {
  STATIC_ASSERT(GuardTag == LogTag::__NO_TAG); // Number of logging tags exceeds maximum supported!
  LogLevelType _level;

public:
  GCErgoNarration(LogLevelType level) : _level(level) {}

  void operator()(const char* fmt, ...) const ATTRIBUTE_PRINTF(2, 3) {
    LogTagSet& ts = LogTagSetMapping<T0, T1, T2, T3, T4>::tagset();
    bool jfr = GCErgoEvent::should_emit(_level);
    bool log = ts.is_level(_level);
    if (!jfr && !log) {
      return;
    }
    if (jfr) {
      char tagbuf[64];
      ts.label(tagbuf, sizeof(tagbuf), "+");
      va_list args;
      va_start(args, fmt);
      GCErgoEvent::commit(_level, tagbuf, fmt, args);
      va_end(args);
    }
    if (log) {
      va_list args;
      va_start(args, fmt);
      ts.vwrite(_level, fmt, args);
      va_end(args);
    }
  }
};

// Single variadic wrapper: log_ergo(Level, subtags...) -> functor. The tags
// after the level are the LOG_TAGS template arguments, so gc+ergo is implied by
// convention at the call site (pass gc, ergo, [subtag]).
#define log_ergo(level, ...) \
  GCErgoNarration<LOG_TAGS(__VA_ARGS__)>(LogLevel::level)

#endif // SHARE_GC_SHARED_GCERGOEVENT_HPP
