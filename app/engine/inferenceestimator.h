// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QFileInfo>
#include <QDir>
#include <QtQml/QQmlEngine>

/**
 * @class   InferenceEstimator
 * @brief   Calculates estimated hardware memory, disk usage, and latency metrics across AI modalities.
 **/
class InferenceEstimator : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString engineType READ engineType WRITE setEngineType NOTIFY parametersChanged)
    Q_PROPERTY(QString promptText READ promptText WRITE setPromptText NOTIFY parametersChanged)
    Q_PROPERTY(int steps READ steps WRITE setSteps NOTIFY parametersChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY parametersChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY parametersChanged)
    Q_PROPERTY(QStringList referenceFilePaths READ referenceFilePaths WRITE setReferenceFilePaths NOTIFY parametersChanged)

    // Output Estimates
    Q_PROPERTY(double totalDiskSizeGb READ totalDiskSizeGb NOTIFY estimatesUpdated)
    Q_PROPERTY(double estimatedPeakRamGb READ estimatedPeakRamGb NOTIFY estimatesUpdated)
    Q_PROPERTY(int estimatedTotalSeconds READ estimatedTotalSeconds NOTIFY estimatesUpdated)
    Q_PROPERTY(QString estimatedTimeFormatted READ estimatedTimeFormatted NOTIFY estimatesUpdated)
    Q_PROPERTY(QVariantList breakdownItems READ breakdownItems NOTIFY estimatesUpdated)

public:
    explicit InferenceEstimator(QObject *parent = nullptr);
    ~InferenceEstimator() override = default;

    QString engineType() const { return m_engineType; }
    void setEngineType(const QString &type);

    QString promptText() const { return m_promptText; }
    void setPromptText(const QString &text);

    int steps() const { return m_steps; }
    void setSteps(int steps);

    int width() const { return m_width; }
    void setWidth(int width);

    int height() const { return m_height; }
    void setHeight(int height);

    QStringList referenceFilePaths() const { return m_referenceFilePaths; }
    void setReferenceFilePaths(const QStringList &paths);

    double totalDiskSizeGb() const { return m_totalDiskSizeGb; }
    double estimatedPeakRamGb() const { return m_estimatedPeakRamGb; }
    int estimatedTotalSeconds() const { return m_estimatedTotalSeconds; }
    QString estimatedTimeFormatted() const { return m_estimatedTimeFormatted; }
    QVariantList breakdownItems() const { return m_breakdownItems; }

    Q_INVOKABLE void setReferenceDirectory(const QString &dirPath);
    Q_INVOKABLE void recalculate();

signals:
    void parametersChanged();
    void estimatesUpdated();

private:
    QString m_engineType = QStringLiteral("image"); // "script", "image", "video", "audio_kokoro", "audio_cpp"
    QString m_promptText;
    int m_steps = 4;
    int m_width = 512;
    int m_height = 512;
    QStringList m_referenceFilePaths;

    double m_totalDiskSizeGb = 0.0;
    double m_estimatedPeakRamGb = 7.45;
    int m_estimatedTotalSeconds = 0;
    QString m_estimatedTimeFormatted;
    QVariantList m_breakdownItems;

    qint64 getFileSize(const QString &filePath) const;
    qint64 getDirFileSize(const QString &dirPath, const QStringList &filters) const;
};

