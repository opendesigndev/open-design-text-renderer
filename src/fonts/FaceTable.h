#pragma once

#include "../text-renderer/Face.h"

#include "../common/buffer_view.h"

#include <string>
#include <unordered_map>

namespace odtr {

/**
 * A table containing FreeType faces
 */
class FaceTable
{
public:
    struct Item
    {
        FacePtr face;
        std::string storageKey;
        bool fallback;
    };

    FaceTable() = default;
    FaceTable(const FaceTable&) = delete;
    FaceTable& operator=(const FaceTable&) = delete;

    void initialize(FreetypeHandle& ft);

    struct LoadFaceAsResultRec
    {
        /**
         * Original postScript name as found in the font file.
         */
        std::string originalFaceName;

        /**
         * Key used to look up the face in this table.
         */
        std::string faceKey;
    };
    using LoadFaceAsResult = Result<LoadFaceAsResultRec, bool>;

    /**
     * @param   storageKey  key under which the data is stored in  @a odtr::FontStorage
     * @param   faceKey     key to look up the face in this table, if empty, the original
     *                      postScriptname from the font file is used
     * @param   faceName    postScript name of the face to load from the buffer,
     *                      if empty, the first face is loaded
     *
     * @return  @a LoadFaceAsResult
     */
    LoadFaceAsResult loadFace(const std::string& storageKey, const std::string& faceKey,
                              const std::string& faceName, BufferView fontData);

    FacesNames loadFaces(const std::string& storageKey, const FacesNames& faces, BufferView fontData);
    FacesNames loadAllFaces(const std::string& storageKey, BufferView fontData);

    void unloadFacesByStorageKey(const std::string& storageKey);

    void discardFaces();
    bool exists(const std::string& name) const;
    const Item* getFaceItem(const std::string& name) const;

    FacesNames listAllFacesNames() const;
    FacesNames listFacesInStorage(const std::string& storageKey) const;

private:
    bool loadItem(const std::string& name, Item item);

    using TableType = std::unordered_map<std::string, Item>;

    FreetypeHandle* ft_ = nullptr;
    TableType faceItems_; ///< The key is a Postscript face name
};

} // namespace odtr
