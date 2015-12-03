/*
  LedDimmer.h - LedDimmer library for dimming Led lights smoothely.
  By Panos Toumpaniaris.  MIT License.
*/

// ensure this library description is only included once
#ifndef LedDimmer_h
#define LedDimmer_h


// library interface description
class LedDimmer
{
    
  // user-accessible "public" interface
  public:

    LedDimmer(int pin);

    int calcStep(const int duration, const int to);
    void update(void);
    void dim(const int step_temp, const int to_Level);

  protected:

    int led;
    int toLevel;
    int curLevel;
    int step;
    int delta;
    unsigned long prevMillis;
    unsigned long curMillis;

};

#endif

