#include "MicroBitCustomConfig.h"

#if __cplusplus <= 199711L
  #error The glue layer requires C++11 support. Please use GCC 4.9.3 or greater.
#endif

#ifndef __MICROBIT_TOUCHDEVELOP_H
#define __MICROBIT_TOUCHDEVELOP_H

#include <climits>
#include <cmath>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <utility>

#include "MicroBit.h"
#include "MicroBitImage.h"
#include "ManagedString.h"
#include "ManagedType.h"

#define TD_NOOP(...)
#define TD_ID(x) x

namespace touch_develop {

  // ---------------------------------------------------------------------------
  // Base definitions that may be referred to by the C++ compiler.
  // ---------------------------------------------------------------------------

  enum TdError {
    TD_UNINITIALIZED_OBJECT_TYPE = 40,
    TD_OUT_OF_BOUNDS,
    TD_BAD_USAGE,
    TD_CONTRACT_ERROR,
  };

  namespace touch_develop {
    ManagedString mk_string(char* c);

    template <typename T>
    inline bool is_null(T* p) {
      return p == NULL;
    }
  }


  // ---------------------------------------------------------------------------
  // An adapter for the API expected by the run-time.
  // ---------------------------------------------------------------------------

  /**
   * The DAL API for [MicroBitMessageBus::listen] takes a [T*] and a
   * [void (T::*method)(MicroBitEvent e)]. The TouchDevelop-to-C++ compiler
   * generates either:
   * - a [void()] (callback that does not capture variables)
   * - a [std::function<void()>] (callback that does capture variables)
   * - a [void(int)] (where the integer is the [value] field of the event)
   * - a [std::function<void(int)>] (same as above with capture)
   *
   * The purpose of this class is to provide constructors for all the types
   * above and a [run] method (suitable for passing to
   * [MicroBitMessageBus::listen]). Implicit conversions make sure the two
   * constructors below also work when passed a bare function pointer.
   *
   * NB: this could be done with a bunch of (void(*)(void*)) casts like I did
   * with [forever_helper], but the version with the class has no casts, which
   * is nicer imho.
   */
  class DalAdapter {
    public:
      explicit DalAdapter(std::function<void()> f);
      explicit DalAdapter(std::function<void(int)> f);
      void run(MicroBitEvent e);

    private:
      const std::function<void(MicroBitEvent)> impl_;
  };

  extern std::map<std::pair<int, int>, unique_ptr<DalAdapter>> handlersMap;

  template <typename T> // T: std::function<void()> or T: std::function<void(int)>
  inline void registerWithDal(int id, int event, T f) {
    // This function implements the TouchDevelop semantics (a.k.a. at most one
    // event handler for each button/event pair) using the global
    // [handlersMap] to un-register any previous event handlers.
    if (f != NULL) {
      auto old_adapter = handlersMap.find({ id, event });
      if (old_adapter != handlersMap.end())
        // If there was something in the table already, un-register the event
        // handler with the DAL.
        uBit.MessageBus.ignore(id, event, old_adapter->second.get(), &DalAdapter::run);

      DalAdapter* new_adapter = new DalAdapter(f);
      uBit.MessageBus.listen(id, event, new_adapter, &DalAdapter::run);
      handlersMap[{ id, event }] = unique_ptr<DalAdapter>(new_adapter);
    }
  }


  // ---------------------------------------------------------------------------
  // Implementation of the base TouchDevelop types
  // ---------------------------------------------------------------------------

  typedef int Number;
  typedef bool Boolean;
  typedef ManagedString String;
  typedef std::function<void()> Action;
  template <typename T> using Action1 = std::function<void(T)>;
  template <typename T> using Collection_of = ManagedType<vector<T>>;
  template <typename T> using Collection = ManagedType<vector<T>>;

  // A short override of [ManagedType] to make the generated code more compact.
  template <typename T>
  class Ref: public ManagedType<T> {
    public:
      Ref();
  };

  template <typename T>
  Ref<T>::Ref() {
    this->object = new T();
    *(this->ref) = 1;
  }

  // ---------------------------------------------------------------------------
  // Implementation of the base TouchDevelop libraries and operations
  // ---------------------------------------------------------------------------

  namespace contract {
    void assert(bool x, ManagedString msg);
  }

  namespace invalid {
    Action action();
  }

  namespace string {
    ManagedString concat(ManagedString s1, ManagedString s2);

    ManagedString _(ManagedString s1, ManagedString s2);

    ManagedString substring(ManagedString s, int i, int j);

    bool equals(ManagedString s1, ManagedString s2);

    int count(ManagedString s);

    ManagedString at(ManagedString s, int i);

    int to_character_code(ManagedString s);

    int code_at(ManagedString s, int i);

    int to_number(ManagedString s);

    void post_to_wall(ManagedString s);
  }

  namespace action {
    void run(Action a);

    bool is_invalid(Action a);
  }

  namespace action1 {
    template <typename T>
    inline void run(Action1<T> a, T arg) {
      if (a)
        a(arg);
    }

    template <typename T>
    inline bool is_invalid(Action1<T> a) {
      if (a)
        return true;
      else
        return false;
    }
  }

  namespace math {
    int max(int x, int y);
    int min(int x, int y);
    int random(int max);
    // Unspecified behavior for int_min
    int abs(int x);
    int mod (int x, int y);

    int pow(int x, int n);

    int clamp(int l, int h, int x);

    int sqrt(int x);

    int sign(int x);
  }

  namespace number {
    bool lt(int x, int y);
    bool le(int x, int y);
    bool neq(int x, int y);
    bool eq(int x, int y);
    bool gt(int x, int y);
    bool ge(int x, int y);
    int add(int x, int y);
    int subtract(int x, int y);
    int divide(int x, int y);
    int multiply(int x, int y);
    ManagedString to_string(int x);
    ManagedString to_character(int x);
    void post_to_wall(int s);
  }

  namespace bits {
    int or_uint32(int x, int y);
    int and_uint32(int x, int y);
    int xor_uint32(int x, int y);
    int shift_left_uint32(int x, int y);
    int shift_right_uint32(int x, int y);
    int rotate_right_uint32(int x, int y);
    int rotate_left_uint32(int x, int y);
  }

  namespace boolean {
    bool or_(bool x, bool y);
    bool and_(bool x, bool y);
    bool not_(bool x);
    bool equals(bool x, bool y);
    ManagedString to_string(bool x);
  }

  // ---------------------------------------------------------------------------
  // Some extra TouchDevelop libraries (Collection, Ref, ...)
  // ---------------------------------------------------------------------------

  // Parameterized types only work if we have the C++11-style "using" typedef.
  namespace create {
    template<typename T>
    inline Collection_of<T> collection_of() {
      return ManagedType<vector<T>>(new vector<T>());
    }

    template<typename T>
    inline Ref<T> ref_of() {
      return Ref<T>();
    }
  }

  namespace collection {
    template<typename T>
    inline Number count(Collection_of<T> c) {
      if (c.get() != NULL)
        return c->size();
      else
        uBit.panic(TD_UNINITIALIZED_OBJECT_TYPE);
    }

    template<typename T>
    inline void add(Collection_of<T> c, T x) {
      if (c.get() != NULL)
        c->push_back(x);
      else
        uBit.panic(TD_UNINITIALIZED_OBJECT_TYPE);
    }

    // First check that [c] is valid (panic if not), then proceed to check that
    // [x] is within bounds.
    template<typename T>
    inline bool in_range(Collection_of<T> c, int x) {
      if (c.get() != NULL)
        return (0 <= x && x < c->size());
      else
        uBit.panic(TD_UNINITIALIZED_OBJECT_TYPE);
    }

    template<typename T>
    inline T at(Collection_of<T> c, int x) {
      if (in_range(c, x))
        return c->at(x);
      else
        uBit.panic(TD_OUT_OF_BOUNDS);
    }

    template<typename T>
    inline void remove_at(Collection_of<T> c, int x) {
      if (!in_range(c, x))
        return;

      c->erase(c->begin()+x);
    }

    template<typename T>
    inline void set_at(Collection_of<T> c, int x, T y) {
      if (!in_range(c, x))
        return;

      c->at(x) = y;
    }

    template<typename T>
    inline Number index_of(Collection_of<T> c, T x, int start) {
      if (!in_range(c, start))
        return -1;

      int index = -1;
      for (int i = start; i < c->size(); ++i)
        if (c->at(i) == x)
          index = i;
      return index;
    }

    template<typename T>
    inline void remove(Collection_of<T> c, T x) {
      remove_at(c, index_of(c, x, 0));
    }
  }

  namespace ref {
    template<typename T>
    inline T _get(Ref<T> x) {
      return *(x.get());
    }

    template<typename T>
    inline void _set(Ref<T> x, T y) {
      *(x.get()) = y;
    }
  }


  // ---------------------------------------------------------------------------
  // Implementation of the BBC micro:bit features
  // ---------------------------------------------------------------------------

  namespace micro_bit {

    // -------------------------------------------------------------------------
    // Sensors
    // -------------------------------------------------------------------------

    int compassHeading();

    int getAcceleration(int dimension);

    void on_calibrate_required (MicroBitEvent e);


    // -------------------------------------------------------------------------
    // Buttons
    // -------------------------------------------------------------------------

    bool isButtonPressed(int button);
    void onButtonPressedExt(int button, int event, std::function<void()> f);
    void onButtonPressed(int button, std::function<void()> f);

    // -------------------------------------------------------------------------
    // Pins
    // -------------------------------------------------------------------------

    int analogReadPin(MicroBitPin& p);

    void analogWritePin(MicroBitPin& p, int value);

    void setAnalogPeriodUs(MicroBitPin& p, int value);

    int digitalReadPin(MicroBitPin& p);

    void digitalWritePin(MicroBitPin& p, int value);

    bool isPinTouched(MicroBitPin& pin);

    void onPinPressed(int pin, std::function<void()> f);

    // -------------------------------------------------------------------------
    // System
    // -------------------------------------------------------------------------

    void runInBackground(std::function<void()> f);

    void pause(int ms);

    void forever(std::function<void()> f);

    int getCurrentTime();

    int i2c_read(int addr);

    void i2c_write(int addr, char c);

    void i2c_write2(int addr, int c1, int c2);

    // -------------------------------------------------------------------------
    // Screen (reading/modifying the global, mutable state of the display)
    // -------------------------------------------------------------------------

    int getBrightness();

    void setBrightness(int percentage);

    void clearScreen();

    void plot(int x, int y);

    void unPlot(int x, int y);

    bool point(int x, int y);

    // -------------------------------------------------------------------------
    // Images (helpers that create/modify a MicroBitImage)
    // -------------------------------------------------------------------------

    // Argument rewritten by the C++ emitter to be what we need
    MicroBitImage createImage(int w, int h, const uint8_t* bitmap);

    MicroBitImage createImageFromString(ManagedString s);

    void clearImage(MicroBitImage i);

    int getImagePixel(MicroBitImage i, int x, int y);

    void setImagePixel(MicroBitImage i, int x, int y, int value);

    int getImageWidth(MicroBitImage i);

    // -------------------------------------------------------------------------
    // Various "show"-style functions to display and scroll things on the screen
    // -------------------------------------------------------------------------

    void showLetter(ManagedString s);

    void showDigit(int n);

    void scrollNumber(int n, int delay);

    void scrollString(ManagedString s, int delay);

    void plotImage(MicroBitImage i, int offset);

    void plotLeds(int w, int h, const uint8_t* bitmap);

    void showImage(MicroBitImage i, int offset);

    // These have their arguments rewritten by the C++ compiler.
    void showLeds(int w, int h, const uint8_t* bitmap, int delay);

    void scrollImage(MicroBitImage i, int offset, int delay);

    void showAnimation(int w, int h, const uint8_t* bitmap, int ms);

    // -------------------------------------------------------------------------
    // BLE Events
    // -------------------------------------------------------------------------

    void generate_event(int id, int event);

    void on_event(int id, std::function<void*(int)> f);

    namespace events {
      void remote_control(int event);
      void camera(int event);
      void audio_recorder(int event);
      void alert(int event);
    }

    // -------------------------------------------------------------------------
    // Music
    // -------------------------------------------------------------------------

    void enablePitch(MicroBitPin& p);

    void pitch(int freq, int ms);
  }

  // ---------------------------------------------------------------------------
  // The DS1307 real-time clock and its i2c communication protocol
  // ---------------------------------------------------------------------------

  namespace ds1307 {

    uint8_t bcd2bin(uint8_t val);

    uint8_t bin2bcd(uint8_t val);

    // The TouchDevelop type is marked as {shim:} an exactly matches this
    // definition. It's kind of unfortunate that we have to duplicate the
    // definition.
    namespace user_types {
      struct DateTime_ {
        Number seconds;
        Number minutes;
        Number hours;
        Number day;
        Number month;
        Number year;
      };
      typedef ManagedType<DateTime_> DateTime;
    }

    void adjust(user_types::DateTime d);

    user_types::DateTime now();
  }

  // -------------------------------------------------------------------------
  // Called at start-up by the generated code (currently not enabled).
  // -------------------------------------------------------------------------
  void internal_main();
}

#endif

// vim: set ts=2 sw=2 sts=2:
