#pragma once

#include "../common/buffer_view.h"
#include "../text-renderer/types.h"

#include <memory>
#include <string>

namespace odtr {

namespace utils {
class Log;
}

class FaceTable;
class FreetypeHandle;
class FontStorage;

class FontManager
{
public:
    static const std::string DEFAULT_EMOJI_FONT;

    explicit FontManager(const utils::Log& log);

    ~FontManager();

    FontManager(FontManager&& other) = default;

    const odtr::FaceTable& facesTable() const;

    /**
     * Allocs buffer of given size in @a FontStorage and returns pointer to it.
     *
     * @param key   unique resource identifier (typically font name - PostScript name)
     * @param size  buffer size
     *
     * @return  pointer to allocated buffer or nullptr if allocation failed
     */
    BufferView::BytePtr allocFontStorageBuffer(const std::string& keY, std::size_t size);

    bool loadFaceFromStorageAs(const std::string& storageKey, const std::string& faceKey, const std::string& facePostScriptName);

    /**
     * Loads a font given by name from @a FontStorage. Font needs to be allocated
     * via @a allocFontStorage prior this method call.
     *
     * @param key  key used to alloc memory in the storage
     *
     * @return  true if a font face successfully loaded
     */
    bool loadFaceFromStorage(const std::string& key);

    FacesNames loadFacesFromStorage(const std::string& key, const FacesNames& faces);

    /**
     * Loads font from a given filename
     *
     * @param key       unique resource identifier (PostScript name)
     * @param filename  path to font file
     *
     * @return
     */
    bool loadFaceFromFile(const std::string& key, const std::string& filename);

    FacesNames loadFacesFromFile(const std::string& key, const FacesNames& faces, const std::string& filename);

    /**
     * Loads font from a given filename
     *
     * @param fontFilename       font file path (also used as storage key)
     * @param faceKey            key to store the face in the face table (can be empty)
     * @param faceName           face PostScript name as stored in the font file
     *
     * @return  true on success
     */
    bool loadFaceFromFileAs(const std::string& fontFilename, const std::string& faceKey, const std::string& faceName);

    FacesNames listAllFaces() const;
    FacesNames listFacesInStorage(const std::string& storageKey) const;

    bool faceExists(const std::string& faceKey) const;

    bool requiresDefaultEmojiFont() const;

    void setRequiresDefaultEmojiFont();

    /**
     * Loads font from a given buffer
     *
     * @param key      font identifier (typically PostScript name) within @a FontManager
     * @param faceKey  key to store the face in the face table (might be empty)
     * @param faceName face PostScript name as stored in the font file, used to select font from collection, might be empty
     * @param data     font data passed within non-owning view of the buffer
     *
     * @return
     */
    bool loadFaceAs(const std::string& key, const std::string& faceKey, const std::string& faceName, BufferView data);

private:
    bool storeFile(const std::string& storageKey, const std::string& filename, bool replace);

    const utils::Log& log_;

    std::unique_ptr<odtr::FreetypeHandle> ft_;
    std::unique_ptr<odtr::FaceTable> faces_;
    std::unique_ptr<odtr::FontStorage> fontStorage_;

    bool requiresDefaultEmojiFont_;
};

} // namespace odtr
