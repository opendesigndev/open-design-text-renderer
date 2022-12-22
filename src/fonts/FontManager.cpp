#include "FontManager.h"

#include "fonts/FaceTable.h"
#include "fonts/FontStorage.h"

#include "utils/Log.h"

#include <algorithm>
#include <fstream>

namespace textify {

const std::string FontManager::DEFAULT_EMOJI_FONT = "_default_emoji_font_";

FontManager::FontManager(const utils::Log& log)
    : log_(log),
      ft_(std::make_unique<textify::FreetypeHandle>()),
      faces_(std::make_unique<textify::FaceTable>()),
      fontStorage_(std::make_unique<textify::FontStorage>()),
      requiresDefaultEmojiFont_(false)
{
    ft_->initialize();
    faces_->initialize(*ft_);
}

FontManager::~FontManager()
{
    faces_->discardFaces();
    ft_->deinitialize();
}

const textify::FaceTable& FontManager::facesTable() const
{
    return *faces_.get();
}

BufferView::BytePtr FontManager::allocFontStorageBuffer(const std::string& key, std::size_t size)
{
    return fontStorage_->alloc(key, size);
}

bool FontManager::loadFaceFromStorage(const std::string& key)
{
    auto data = fontStorage_->get(key);
    if (data) {
        return loadFaceAs(key, key, "", data);
    }

    return false;
}

bool FontManager::loadFaceFromStorageAs(const std::string& storageKey, const std::string& faceKey, const std::string& facePostScriptName) {
    auto data = fontStorage_->get(storageKey);
    if (!data) {
        return false;
    }

    if (auto result = faces_->loadFace(storageKey, faceKey, facePostScriptName, data)) {
        log_.info("face {} from file {} available under key: {} ",
                       result.value().originalFaceName.c_str(),
                       storageKey.c_str(),
                       result.value().faceKey.c_str()
                 );
        return true;
    }

    return false;
}

FacesNames FontManager::loadFacesFromStorage(const std::string& storageKey, const FacesNames& faces)
{
    auto data = fontStorage_->get(storageKey);
    if (data) {
        auto loadedFaces = faces_->loadFaces(storageKey, faces, data);

        if (std::find(std::begin(loadedFaces), std::end(loadedFaces), storageKey) == std::end(loadedFaces)) {
            // storageKey is not among the loaded faces
            auto faceName = faces.size() == 1 ? faces[0] : "";
            auto loaded = loadFaceAs(storageKey, storageKey, faceName, data);
            if (loaded) {
                loadedFaces.push_back(storageKey);
            }
        }

        return loadedFaces;
    } else {
        log_.error("Failed to load faces from storage: {}", storageKey);
    }
    return {};
}

bool FontManager::loadFaceFromFile(const std::string& key, const std::string& filename)
{
    if (!storeFile(key, filename, true)) {
        return false;
    }

    return loadFaceFromStorage(key);
}

FacesNames FontManager::loadFacesFromFile(const std::string& key, const FacesNames& faces, const std::string& filename)
{
    if (!storeFile(key, filename, true)) {
        return {};
    }

    return loadFacesFromStorage(key, faces);
}

bool FontManager::loadFaceFromFileAs(const std::string& fontFilename, const std::string& faceKey, const std::string& facePostScriptName)
{
    if (!storeFile(fontFilename /* storage key is the filename */,
                   fontFilename,
                   false /* do not override existing records */)) {
        return false;
    }

    return loadFaceFromStorageAs(fontFilename, faceKey, facePostScriptName);
}

FacesNames FontManager::listAllFaces() const
{
    return faces_->listAllFacesNames();
}

FacesNames FontManager::listFacesInStorage(const std::string& storageKey) const
{
    return faces_->listFacesInStorage(storageKey);
}

bool FontManager::faceExists(const std::string& faceKey) const
{
    return faces_->exists(faceKey);
}

bool FontManager::requiresDefaultEmojiFont() const
{
    return requiresDefaultEmojiFont_;
}

void FontManager::setRequiresDefaultEmojiFont()
{
    const bool hasDefaultEmojiFont = faces_->getFaceItem(DEFAULT_EMOJI_FONT) != nullptr;
    if (!hasDefaultEmojiFont) {
        requiresDefaultEmojiFont_ = true;
    }
}

bool FontManager::loadFaceAs(const std::string& storageKey, const std::string& faceKey, const std::string& faceName, BufferView data)
{
    auto result = faces_->loadFace(storageKey, faceKey, faceName, data);
    if (!result) {
        log_.error("Failed to load font face ", faceKey);
    }
    if (faceKey == DEFAULT_EMOJI_FONT) {
        requiresDefaultEmojiFont_ = false;
    }
    return result;
}

bool FontManager::storeFile(const std::string& storageKey, const std::string& filename, bool replace)
{
    if (fontStorage_->contains(storageKey) && !replace) {
        return true;
    }

    std::ifstream ifs(filename, std::ios::binary);

    if (!ifs.is_open()) {
        return false;
    }

    ifs.seekg(0, std::ios::end);
    std::size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto buffer = fontStorage_->alloc(storageKey, size);
    if (!buffer) {
        log_.error("Failed to allocate font storage.");
        return false;
    }

    ifs.read((char*)buffer, size);

    return true;
}

} // namespace textify
