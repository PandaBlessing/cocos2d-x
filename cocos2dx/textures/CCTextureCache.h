/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __CCTEXTURE_CACHE_H__
#define __CCTEXTURE_CACHE_H__

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <string>

#include "cocoa/CCObject.h"
#include "cocoa/CCDictionary.h"
#include "textures/CCTexture2D.h"
#include "platform/CCImage.h"

#if CC_ENABLE_CACHE_TEXTURE_DATA
    #include "platform/CCImage.h"
    #include <list>
#endif

NS_CC_BEGIN

/**
 * @addtogroup textures
 * @{
 */

/** @brief Singleton that handles the loading of textures
* Once the texture is loaded, the next time it will return
* a reference of the previously loaded texture reducing GPU & CPU memory
*/
class CC_DLL TextureCache : public Object
{
public:
    /** Returns the shared instance of the cache */
    static TextureCache * getInstance();

    /** @deprecated Use getInstance() instead */
    CC_DEPRECATED_ATTRIBUTE static TextureCache * sharedTextureCache() { return TextureCache::getInstance(); }

    /** purges the cache. It releases the retained instance.
     @since v0.99.0
     */
    static void destroyInstance();

    /** @deprecated Use destroyInstance() instead */
    CC_DEPRECATED_ATTRIBUTE static void purgeSharedTextureCache() { return TextureCache::destroyInstance(); }

    /** Reload all textures
     It's only useful when the value of CC_ENABLE_CACHE_TEXTURE_DATA is 1
     */
    static void reloadAllTextures();

public:
    TextureCache();
    virtual ~TextureCache();

    const char* description(void) const;

    Dictionary* snapshotTextures();

    /** Returns a Texture2D object given an file image
    * If the file image was not previously loaded, it will create a new Texture2D
    *  object and it will return it. It will use the filename as a key.
    * Otherwise it will return a reference of a previously loaded image.
    * Supported image extensions: .png, .bmp, .tiff, .jpeg, .pvr, .gif
    */
    Texture2D* addImage(const char* fileimage);

    /* Returns a Texture2D object given a file image
    * If the file image was not previously loaded, it will create a new Texture2D object and it will return it.
    * Otherwise it will load a texture in a new thread, and when the image is loaded, the callback will be called with the Texture2D as a parameter.
    * The callback will be called from the main thread, so it is safe to create any cocos2d object from the callback.
    * Supported image extensions: .png, .jpg
    * @since v0.8
    */
    virtual void addImageAsync(const char *path, Object *target, SEL_CallFuncO selector);

    /** Returns a Texture2D object given an UIImage image
    * If the image was not previously loaded, it will create a new Texture2D object and it will return it.
    * Otherwise it will return a reference of a previously loaded image
    * The "key" parameter will be used as the "key" for the cache.
    * If "key" is nil, then a new texture will be created each time.
    */
    Texture2D* addUIImage(Image *image, const char *key);

    /** Returns an already created texture. Returns nil if the texture doesn't exist.
    @since v0.99.5
    */
    Texture2D* textureForKey(const char* key);

    /** Purges the dictionary of loaded textures.
    * Call this method if you receive the "Memory Warning"
    * In the short term: it will free some resources preventing your app from being killed
    * In the medium term: it will allocate more resources
    * In the long term: it will be the same
    */
    void removeAllTextures();

    /** Removes unused textures
    * Textures that have a retain count of 1 will be deleted
    * It is convenient to call this method after when starting a new Scene
    * @since v0.8
    */
    void removeUnusedTextures();

    /** Deletes a texture from the cache given a texture
    */
    void removeTexture(Texture2D* texture);

    /** Deletes a texture from the cache given a its key name
    @since v0.99.4
    */
    void removeTextureForKey(const char *textureKeyName);

    /** Output to CCLOG the current contents of this TextureCache
    * This will attempt to calculate the size of each texture, and the total texture memory in use
    *
    * @since v1.0
    */
    void dumpCachedTextureInfo();
    
    /** Returns a Texture2D object given an PVR filename
    * If the file image was not previously loaded, it will create a new Texture2D
    *  object and it will return it. Otherwise it will return a reference of a previously loaded image
    */
    Texture2D* addPVRImage(const char* filename);
    
    /** Returns a Texture2D object given an ETC filename
     * If the file image was not previously loaded, it will create a new Texture2D
     *  object and it will return it. Otherwise it will return a reference of a previously loaded image
     */
    Texture2D* addETCImage(const char* filename);

private:
    void addImageAsyncCallBack(float dt);
    void loadImage();
    Image::Format computeImageFormatType(std::string& filename);

public:
    struct AsyncStruct
    {
    public:
        AsyncStruct(const std::string& fn, Object *t, SEL_CallFuncO s) : filename(fn), target(t), selector(s) {}

        std::string            filename;
        Object    *target;
        SEL_CallFuncO        selector;
    };

protected:
    typedef struct _ImageInfo
    {
        AsyncStruct *asyncStruct;
        Image        *image;
        Image::Format imageType;
    } ImageInfo;
    
    std::thread* _loadingThread;

    std::queue<AsyncStruct*>* _asyncStructQueue;
    std::queue<ImageInfo*>* _imageInfoQueue;

    std::mutex _asyncStructQueueMutex;
    std::mutex _imageInfoMutex;

    std::mutex _sleepMutex;
    std::condition_variable _sleepCondition;

    bool _needQuit;

    int _asyncRefCount;

    Dictionary* _textures;

    static TextureCache *_sharedTextureCache;
};

#if CC_ENABLE_CACHE_TEXTURE_DATA

class VolatileTexture
{
    typedef enum {
        kInvalid = 0,
        kImageFile,
        kImageData,
        kString,
        kImage,
    }ccCachedImageType;

public:
    VolatileTexture(Texture2D *t);
    ~VolatileTexture();

    static void addImageTexture(Texture2D *tt, const char* imageFileName, Image::Format format);
    static void addStringTexture(Texture2D *tt, const char* text, const FontDefinition& fontDefinition);
    static void addDataTexture(Texture2D *tt, void* data, Texture2D::PixelFormat pixelFormat, const Size& contentSize);
    static void addImage(Texture2D *tt, Image *image);

    static void setTexParameters(Texture2D *t, const ccTexParams &texParams);
    static void removeTexture(Texture2D *t);
    static void reloadAllTextures();

public:
    static std::list<VolatileTexture*> _textures;
    static bool _isReloading;
    
private:
    // find VolatileTexture by Texture2D*
    // if not found, create a new one
    static VolatileTexture* findVolotileTexture(Texture2D *tt);

protected:
    Texture2D *_texture;
    
    Image *_uiImage;

    ccCachedImageType _cashedImageType;

    void *_textureData;
    Size _textureSize;
    Texture2D::PixelFormat _pixelFormat;

    std::string _fileName;
    Image::Format _fmtImage;

    ccTexParams      _texParams;
    std::string      _text;
    FontDefinition   _fontDefinition;
};

#endif

// end of textures group
/// @}

NS_CC_END

#endif //__CCTEXTURE_CACHE_H__

