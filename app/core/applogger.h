// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QtGlobal>
#include <QString>

/**
 * @namespace AppLogger
 * @brief Thread-safe global message interceptor for disk-backed diagnostics.
 *
 * AppLogger intercepts Qt's global message macros (`qDebug()`, `qInfo()`, `qWarning()`, `qCritical()`)
 * and routes them safely to a rotating disk file with immediate flushing to guarantee
 * crash diagnostics are preserved.
 */
namespace AppLogger {

/**
 * @brief Installs the global message handler and opens the diagnostic log file.
 * @param customLogDir Optional explicit directory path. If empty, defaults to the OS-standard AppData location.
 */
void initLogging(const QString &customLogDir = QString());

/**
 * @brief Flushes the buffer, closes the file handle, and restores standard output routing.
 */
void closeLogging();

/**
 * @brief Retrieves the absolute path to the active `.log` file on disk.
 * @return Absolute string path, or an empty string if logging is inactive.
 */
QString getLogFilePath();

/**
 * @brief Checks if the custom message handler is actively installed and routing output to disk.
 * @return True if initialized, false otherwise.
 */
bool isLoggingInitialized();

} // namespace AppLogger
