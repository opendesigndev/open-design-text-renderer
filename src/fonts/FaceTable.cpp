#include "FaceTable.h"

#include <algorithm>

namespace {

bool wantFace(const FacesNames& faces, const std::string& name) {
    if (faces.empty()) {
        return true;
    }

    return std::find(std::begin(faces), std::end(faces), name) != std::end(faces);
}

}

namespace odtr {

struct FT_FaceHandle
{
    FT_FaceHandle(FreetypeHandle* ft, BufferView data)
    {
        FT_Open_Args openArgs;
        openArgs.flags = FT_OPEN_MEMORY;
        openArgs.memory_base = data.data();
        openArgs.memory_size = FT_Long(data.size());

        error = FT_Open_Face(*ft, &openArgs, -1, &face);
    }

    ~FT_FaceHandle()
    {
        FT_Done_Face(face);
    }

    /* implicit */ operator bool() const
    {
        return error == FT_Err_Ok;
    }

    FT_Face& operator->() {
        return face;
    }

    FT_Face face;
    FT_Error error;
};

void FaceTable::initialize(FreetypeHandle& ft)
{
    ft_ = &ft;
}

FaceTable::LoadFaceAsResult FaceTable::loadFace(const std::string& storageKey, const std::string& faceKey,
                                                const std::string& faceName, BufferView fontData)
{
    FT_FaceHandle faceHandle(ft_, fontData);
    if (!faceHandle) {
        return false;
    }

    for (auto faceIdx = 0; faceIdx < faceHandle->num_faces; ++faceIdx) {
        FacePtr facePtr(new Face(*ft_, fontData.data(), static_cast<int>(fontData.size()), faceIdx));
        const auto& origName = facePtr->getPostScriptName();

        if (origName == faceName || faceName.empty()) {
            const auto& key = faceKey.empty() ? origName : faceKey;
            if (!loadItem(key, {facePtr, storageKey, false})) {
                return false;
            }
            return LoadFaceAsResultRec{origName, key};
        }
    }

    return false;
}


FacesNames FaceTable::loadFaces(const std::string& storageKey, const FacesNames& faces, BufferView fontData)
{
    FT_FaceHandle faceHandle(ft_, fontData);
    if (!faceHandle) {
        return {};
    }

    FacesNames loadedNames;

    for (auto faceIdx = 0; faceIdx < faceHandle->num_faces; ++faceIdx) {
        FacePtr facePtr(new Face(*ft_, fontData.data(), static_cast<int>(fontData.size()), faceIdx));
        const auto& name = facePtr->getPostScriptName();

        if(wantFace(faces, name) && loadItem(name, {facePtr, storageKey, false})) {
            loadedNames.push_back(name);
        }
    }

    return loadedNames;
}

FacesNames FaceTable::loadAllFaces(const std::string& storageKey, BufferView fontData) {
    return loadFaces(storageKey, {}, fontData);
}

void FaceTable::unloadFacesByStorageKey(const std::string& storageKey)
{
    for (auto it = std::begin(faceItems_); it != std::end(faceItems_); ++it) {
        if (it->second.storageKey == storageKey) {
            it->second.face.destroy();
        }
        it = faceItems_.erase(it);
    }
}

void FaceTable::discardFaces()
{
    for (auto& item : faceItems_)
        item.second.face.destroy();
    faceItems_.clear();
}

bool FaceTable::exists(const std::string& name) const
{
    return faceItems_.find(name) != std::end(faceItems_);
}

const FaceTable::Item* FaceTable::getFaceItem(const std::string& name) const
{
    auto it = faceItems_.find(name);
    if (it == std::end(faceItems_)) {
        return nullptr;
    }

    return &it->second;
}

FacesNames FaceTable::listAllFacesNames() const
{
    FacesNames fontNames;

    for (const auto& item : faceItems_) {
        fontNames.push_back(item.first);
    }

    return fontNames;
}

FacesNames FaceTable::listFacesInStorage(const std::string& storageKey) const
{
    FacesNames fontNames;

    for (const auto& item : faceItems_) {
        const auto& [faceName, faceRec] = item;

        if (faceRec.storageKey == storageKey) {
            fontNames.push_back(faceName);
        }
    }

    return fontNames;
}

bool FaceTable::loadItem(const std::string& name, Item item)
{
    auto oldItemIt = faceItems_.find(name);
    if (oldItemIt != std::end(faceItems_)) {
        oldItemIt->second.face.destroy();
    }

    faceItems_[name] = item;

    return item.face->ready();
}
} // namespace odtr
