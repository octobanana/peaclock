#ifndef PEACLOCK_HH
#define PEACLOCK_HH

#include <string>

class Peaclock
{
public:

  Peaclock();
  ~Peaclock();

  void run();

private:

  void event_loop();
  int ctrl_key(int c);
  std::string readline(std::string const& prompt, bool& is_running);
};

#endif // PEACLOCK_HH
