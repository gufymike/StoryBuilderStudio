// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "storyboardmanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <algorithm>

StoryBoardManager::StoryBoardManager(QObject *parent)
    : QObject(parent)
{
}

void StoryBoardManager::setEpisodePath(const QString &path)
{
    QString native = QDir::toNativeSeparators(path);
    if (m_episodePath != native) {
        m_episodePath = native;
        emit episodePathChanged();
        emit hasValidPathChanged();
        load();
    }
}

QString StoryBoardManager::getJsonFilePath() const
{
    if (m_episodePath.isEmpty()) return QString();
    return QDir(m_episodePath).filePath(QStringLiteral("storyboard.json"));
}

void StoryBoardManager::normalizeOrders()
{
    for (int i = 0; i < m_shots.size(); ++i) {
        QVariantMap map = m_shots[i].toMap();
        map[QStringLiteral("order")] = i;
        m_shots[i] = map;
    }
}

void StoryBoardManager::load()
{
    m_shots.clear();
    QString filePath = getJsonFilePath();
    if (filePath.isEmpty()) {
        emit shotsChanged();
        return;
    }

    if (!QFile::exists(filePath)) {
        QFile f(filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit boardError(tr("Unable to create storyboard: %1").arg(filePath));
            return;
        }
        f.close();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit boardError(tr("Unable to open storyboard: %1").arg(filePath));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        emit shotsChanged();
        return;
    }

    QJsonArray array = doc.object().value(QStringLiteral("shots")).toArray();
    for (const QJsonValue &v : array) {
        m_shots.append(v.toObject().toVariantMap());
    }

    std::sort(m_shots.begin(), m_shots.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("order"), 0).toInt() < b.toMap().value(QStringLiteral("order"), 0).toInt();
    });

    normalizeOrders();
    emit shotsChanged();
}

void StoryBoardManager::save()
{
    QString filePath = getJsonFilePath();
    if (filePath.isEmpty()) {
        emit boardError(tr("Cannot save: No episode path designated."));
        return;
    }

    QJsonObject root;
    root[QStringLiteral("version")] = QStringLiteral("1.0");
    root[QStringLiteral("last_modified")] = QDateTime::currentDateTime().toString(Qt::ISODate);

    normalizeOrders();

    QJsonArray array;
    for (const QVariant &v : m_shots) {
        array.append(QJsonObject::fromVariantMap(v.toMap()));
    }
    root[QStringLiteral("shots")] = array;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit boardError(tr("Failed to save storyboard.json: %1").arg(file.errorString()));
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    emit boardSaved();
}

bool StoryBoardManager::addShot(const QString &name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        emit boardError(tr("Shot name cannot be empty."));
        return false;
    }

    for (const QVariant &v : m_shots) {
        if (v.toMap().value(QStringLiteral("name")).toString().compare(trimmed, Qt::CaseInsensitive) == 0) {
            emit boardError(tr("A shot named '%1' already exists.").arg(trimmed));
            return false;
        }
    }

    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    QVariantMap shot;
    shot[QStringLiteral("id")] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    shot[QStringLiteral("name")] = trimmed;
    shot[QStringLiteral("order")] = m_shots.size();
    shot[QStringLiteral("created_at")] = now;
    shot[QStringLiteral("modified_at")] = now;
    shot[QStringLiteral("key_image")] = QString();
    shot[QStringLiteral("final_shot")] = QString();
    shot[QStringLiteral("script_file")] = QString();
    shot[QStringLiteral("images")] = QVariantList();
    shot[QStringLiteral("audio")] = QVariantList();
    shot[QStringLiteral("videos")] = QVariantList();
    shot[QStringLiteral("scripts")] = QVariantList();

    m_shots.append(shot);
    save();
    emit shotsChanged();
    return true;
}

bool StoryBoardManager::removeShot(int index)
{
    if (index < 0 || index >= m_shots.size()) return false;
    m_shots.removeAt(index);
    normalizeOrders();
    save();
    emit shotsChanged();
    return true;
}

bool StoryBoardManager::moveShot(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_shots.size() ||
        toIndex < 0 || toIndex >= m_shots.size() || fromIndex == toIndex) {
        return false;
    }

    m_shots.move(fromIndex, toIndex);
    normalizeOrders();
    save();
    emit shotsChanged();
    return true;
}

bool StoryBoardManager::moveShotUp(int index)
{
    return moveShot(index, index - 1);
}

bool StoryBoardManager::moveShotDown(int index)
{
    return moveShot(index, index + 1);
}

bool StoryBoardManager::updateShotField(int index, const QString &key, const QVariant &value)
{
    if (index < 0 || index >= m_shots.size()) return false;
    QVariantMap map = m_shots[index].toMap();
    map[key] = value;
    map[QStringLiteral("modified_at")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_shots[index] = map;
    save();
    emit shotsChanged();
    return true;
}

bool StoryBoardManager::renameShot(int index, const QString &newName)
{
    QString trimmed = newName.trimmed();
    if (trimmed.isEmpty()) return false;
    for (int i = 0; i < m_shots.size(); ++i) {
        if (i != index && m_shots[i].toMap().value(QStringLiteral("name")).toString().compare(trimmed, Qt::CaseInsensitive) == 0) {
            emit boardError(tr("A shot named '%1' already exists.").arg(trimmed));
            return false;
        }
    }
    return updateShotField(index, QStringLiteral("name"), trimmed);
}

bool StoryBoardManager::setShotKeyImage(int index, const QString &filePath)
{
    return updateShotField(index, QStringLiteral("key_image"), filePath);
}

bool StoryBoardManager::setShotFinalCut(int index, const QString &filePath)
{
    return updateShotField(index, QStringLiteral("final_shot"), filePath);
}

bool StoryBoardManager::setShotScript(int index, const QString &filePath)
{
    return updateShotField(index, QStringLiteral("script_file"), filePath);
}

bool StoryBoardManager::addAssetToShot(int index, const QString &category, const QString &filePath)
{
    if (index < 0 || index >= m_shots.size() || filePath.isEmpty()) return false;

    QVariantMap map = m_shots[index].toMap();
    QVariantList list = map.value(category).toList();

    if (!list.contains(filePath)) {
        list.append(filePath);
        map[category] = list;

        // Auto-assign first image as key_image if unset
        if (category == QStringLiteral("images") && map.value(QStringLiteral("key_image")).toString().isEmpty()) {
            map[QStringLiteral("key_image")] = filePath;
        }

        map[QStringLiteral("modified_at")] = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_shots[index] = map;
        save();
        emit shotsChanged();
        return true;
    }
    return false;
}

bool StoryBoardManager::removeAssetFromShot(int index, const QString &category, const QString &filePath)
{
    if (index < 0 || index >= m_shots.size()) return false;

    QVariantMap map = m_shots[index].toMap();
    QVariantList list = map.value(category).toList();
    if (list.removeAll(filePath) > 0) {
        map[category] = list;

        if (map.value(QStringLiteral("key_image")).toString() == filePath) {
            map[QStringLiteral("key_image")] = list.isEmpty() ? QString() : list.first().toString();
        }
        if (map.value(QStringLiteral("final_shot")).toString() == filePath) {
            map[QStringLiteral("final_shot")] = QString();
        }

        map[QStringLiteral("modified_at")] = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_shots[index] = map;
        save();
        emit shotsChanged();
        return true;
    }
    return false;
}

void StoryBoardManager::cleanupDeletedAsset(const QString &filePath)
{
    if (filePath.isEmpty() || m_shots.isEmpty()) return;

    QString cleanTarget = QDir::toNativeSeparators(filePath).toLower();
    bool changed = false;

    for (int i = 0; i < m_shots.size(); ++i) {
        QVariantMap map = m_shots[i].toMap();

        auto pruneList = [&](const QString &category) {
            QVariantList list = map.value(category).toList();
            for (int j = list.size() - 1; j >= 0; --j) {
                if (QDir::toNativeSeparators(list[j].toString()).toLower() == cleanTarget) {
                    list.removeAt(j);
                    changed = true;
                }
            }
            map[category] = list;
        };

        pruneList(QStringLiteral("images"));
        pruneList(QStringLiteral("audio"));
        pruneList(QStringLiteral("videos"));
        pruneList(QStringLiteral("scripts"));

        if (QDir::toNativeSeparators(map.value(QStringLiteral("key_image")).toString()).toLower() == cleanTarget) {
            QVariantList imgs = map.value(QStringLiteral("images")).toList();
            map[QStringLiteral("key_image")] = imgs.isEmpty() ? QString() : imgs.first().toString();
            changed = true;
        }

        if (QDir::toNativeSeparators(map.value(QStringLiteral("final_shot")).toString()).toLower() == cleanTarget) {
            map[QStringLiteral("final_shot")] = QString();
            changed = true;
        }

        m_shots[i] = map;
    }

    if (changed) {
        save();
        emit shotsChanged();
    }
}

