#pragma once

#include <string>
#include <unordered_map>
#include <queue>

namespace tk { namespace ui {

struct ImageInfo
{
  uint32_t width{};
  uint32_t height{};
  uint32_t index{};
  uint32_t channel{};
};

class ImageManager
{
private:
  ImageManager()                           = default;
  ~ImageManager()                          = default;
public:
  ImageManager(ImageManager const&)            = delete;
  ImageManager(ImageManager&&)                 = delete;
  ImageManager& operator=(ImageManager const&) = delete;
  ImageManager& operator=(ImageManager&&)      = delete;

  static auto instance() noexcept -> ImageManager&
  {
    static ImageManager instance;
    return instance;
  }

  void load(std::string_view path) noexcept;

  // TODO: only call when images so much even exceed gpu memory
  void unload(std::string_view path) noexcept;

  auto contains(std::string_view path) const noexcept { return _slot_indexs.contains(path.data()); }

  auto at(std::string_view path) const noexcept -> ImageInfo const& { return _slots.at(_slot_indexs.at(path.data())).info; }

private:
  struct Slot
  {
    ImageInfo info;
  };
  std::vector<Slot>                         _slots;
  std::unordered_map<std::string, uint32_t> _slot_indexs;
  std::queue<uint32_t>                      _free_slots;
};

inline static auto& g_img_mgr{ ImageManager::instance() };

}}
