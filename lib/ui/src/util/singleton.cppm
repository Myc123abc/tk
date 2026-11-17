export module tk.singleton;

export namespace tk {

template <typename T>
class Singleton
{
protected:
  Singleton()                            = default;
  ~Singleton()                           = default;
public:
  Singleton(Singleton const&)            = delete;
  Singleton(Singleton&&)                 = delete;
  Singleton& operator=(Singleton const&) = delete;
  Singleton& operator=(Singleton&&)      = delete;

  static T& instance() noexcept
  {
    static T value;
    return value;
  }
};

}
