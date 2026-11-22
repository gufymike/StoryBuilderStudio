/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: applogger.cpp
** Thread-safe file-backed logging subsystem.
**
****************************************************************************/
#include "applogger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRecursiveMutex>  // <-- Replace QMutex
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <iostream>

namespace {
QFile s_logFile;
QRecursiveMutex s_logMutex;  // <-- Must be recursive
bool s_initialized = false;
QtMessageHandler s_previousHandler = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QMutexLocker<QRecursiveMutex> locker(&s_logMutex);

    QString levelTag;
    switch (type) {
    case QtDebugMsg:    levelTag = QStringLiteral("[DEBUG]"); break;
    case QtInfoMsg:     levelTag = QStringLiteral("[INFO] "); break;
    case QtWarningMsg:  levelTag = QStringLiteral("[WARN] "); break;
    case QtCriticalMsg: levelTag = QStringLiteral("[CRIT] "); break;
    case QtFatalMsg:    levelTag = QStringLiteral("[FATAL]"); break;
    }

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    QString logLine = QStringLiteral("%1 %2 %3\n").arg(timestamp, levelTag, msg);

    if (s_logFile.isOpen()) {
        s_logFile.write(logLine.toUtf8());
        s_logFile.flush();
    }

    std::cerr << logLine.toLocal8Bit().constData();
}
}

namespace AppLogger {

void initLogging(const QString &customLogDir)
{
    QMutexLocker<QRecursiveMutex> locker(&s_logMutex);
    if (s_initialized) return;

    QString baseDir = customLogDir;
    if (baseDir.isEmpty()) {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!baseDir.endsWith(QStringLiteral("storyBuilderStudio"), Qt::CaseInsensitive)) {
            baseDir += QStringLiteral("/storyBuilderStudio");
        }
        baseDir += QStringLiteral("/logs");
    }

    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QString filePath = dir.absoluteFilePath(QStringLiteral("story_builder_studio.log"));
    s_logFile.setFileName(filePath);

    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        s_initialized = true;
        s_previousHandler = qInstallMessageHandler(messageHandler);

        // This call will now safely re-enter messageHandler on the same thread
        qInfo() << "==================================================";
        qInfo() << "Story Builder Studio Log Session Started";
        qInfo() << "Log target:" << filePath;
        qInfo() << "==================================================";
    }
}

void closeLogging()
{
    QMutexLocker<QRecursiveMutex> locker(&s_logMutex);
    if (!s_initialized) return;

    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;

    if (s_logFile.isOpen()) {
        s_logFile.flush();
        s_logFile.close();
    }
    s_initialized = false;
}

QString getLogFilePath()
{
    QMutexLocker<QRecursiveMutex> locker(&s_logMutex);
    return s_logFile.fileName();
}

bool isLoggingInitialized()
{
    QMutexLocker<QRecursiveMutex> locker(&s_logMutex);
    return s_initialized;
}

}

