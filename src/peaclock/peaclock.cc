#include "peaclock/peaclock.hh"

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

Peaclock::Peaclock()
{
}

Peaclock::~Peaclock()
{
}

void Peaclock::run()
{
  std::cout
  << aec::cursor_hide
  << aec::erase_screen
  << aec::cursor_home
  << std::flush;

  // set terminal mode to raw
  OB::Term::Mode term_mode;
  term_mode.set_raw();

  // start the event loop
  event_loop();

  std::cout
  << aec::erase_screen
  << aec::cursor_home
  << aec::cursor_show
  << std::flush;
}

void Peaclock::event_loop()
{
  // control when to exit the event loop
  bool is_running {true};

  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};

  // command prompt strings
  std::string prompt {aec::wrap(":", aec::fg_white)};
  int prompt_clear_sec {2};
  int prompt_clear {0};

  // each loop iteration should take around 1 second
  while (is_running)
  {
    OB::Term::size(width, height);

    if (prompt_clear > 0)
    {
      --prompt_clear;
      std::cout
      << aec::erase_up
      << aec::cursor_home
      << std::flush;
    }
    else
    {
      std::cout
      << aec::erase_screen
      << aec::cursor_home
      << std::flush;
    }

    // handle draw
    {
    }

    // handle input
    {
      char c {0};
      int num_read {0};

      int loop {20};
      auto wait {std::chrono::milliseconds(50)};

      while (is_running && loop-- > 0)
      {
        while ((num_read = read(STDIN_FILENO, &c, 1)) == 1)
        {
          if (num_read == -1 && errno != EAGAIN)
          {
            throw std::runtime_error("read failed");
          }

          // ctrl-c
          if (static_cast<int>(c) == ctrl_key('c'))
          {
            is_running = false;
            break;
          }

          // quit
          else if (c == 'q' || c == 'Q')
          {
            is_running = false;
            break;
          }

          // command prompt
          else if (c == ':')
          {
            std::size_t x {0};
            std::size_t y {0};
            aec::cursor_get(x, y, false);

            std::cout
            << aec::cursor_set(0, height)
            << aec::erase_line
            << aec::cursor_show
            << std::flush;

            // read user input
            auto input {readline(prompt, is_running)};

            std::cout
            << aec::cursor_hide
            << aec::cr
            << aec::erase_line;

            if (input == "q" || ! is_running)
            {
              is_running = false;
              loop = 0;
              break;
            }
            else if (input.empty())
            {
            }
            else
            {
              std::cout
              << aec::wrap("?", aec::fg_white)
              << aec::wrap(input, aec::fg_red);
              prompt_clear = prompt_clear_sec;
            }

            std::cout
            << aec::cursor_set(x, y)
            << std::flush;

            loop = 0;
            break;
          }
        }

        std::this_thread::sleep_for(wait);
      }
    }
  }
}

int Peaclock::ctrl_key(int c)
{
  return (c & 0x1f);
}

std::string Peaclock::readline(std::string const& prompt, bool& is_running)
{
  std::string input;
  std::size_t index {0};

  char seq[3];

  char c {0};
  int num_read {0};

  bool loop {true};
  auto wait {std::chrono::milliseconds(50)};

  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};
  OB::Term::size(width, height);

  std::cout
  << prompt
  << std::flush;

  while (loop && is_running)
  {
    while ((num_read = read(STDIN_FILENO, &c, 1)) == 1)
    {
      if (num_read == -1 && errno != EAGAIN)
      {
        throw std::runtime_error("read failed");
      }

      // esc / esc sequence
      if (static_cast<int>(c) == 27)
      {
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
        {
          // exit the command prompt
          loop = false;
          input.clear();
          break;
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1)
        {
          break;
        }

        if (seq[0] == '[')
        {
          if (seq[1] >= '0' && seq[1] <= '9')
          {
            if (read(STDIN_FILENO, &seq[2], 1) != 1)
            {
              break;
            }

            if (seq[2] == '~')
            {
              switch (seq[1])
              {
                case '3':
                {
                  // key_del
                  // erase char under cursor
                  if (index < input.size())
                  {
                    input.erase(index, 1);

                    std::cout
                    << aec::cursor_hide
                    << aec::cursor_save
                    << aec::cr
                    << aec::erase_line
                    << prompt
                    << input
                    << aec::cursor_load
                    << aec::cursor_show
                    << std::flush;
                  }

                  // exit the command prompt
                  else if (input.empty())
                  {
                    loop = false;
                    input.clear();
                    break;
                  }

                  break;
                }

                default:
                {
                  break;
                }
              }

              break;
            }
          }
          else
          {
            switch (seq[1])
            {
              case 'A':
              {
                // key_up
                // cycle backwards in history
                break;
              }

              case 'B':
              {
                // key_down
                // cycle forwards in history
                break;
              }

              case 'C':
              {
                // key_right
                // move cursor right
                if (index < input.size())
                {
                  ++index;
                  std::cout
                  << aec::cursor_right
                  << std::flush;
                }
                break;
              }

              case 'D':
              {
                // key_left
                // move cursor left
                if (index)
                {
                  --index;
                  std::cout
                  << aec::cursor_left
                  << std::flush;
                }
                break;
              }

              default:
              {
                break;
              }
            }

            break;
          }
        }

        break;
      }

      // ctrl-c
      if (static_cast<int>(c) == ctrl_key('c'))
      {
        // exit the main event loop
        is_running = false;
        input.clear();
        break;
      }

      // ctrl-d
      if (static_cast<int>(c) == ctrl_key('d'))
      {
        // submit the input string
        loop = false;
        break;
      }

      // ctrl-f
      if (static_cast<int>(c) == ctrl_key('f'))
      {
        // move cursor right
        if (index < input.size())
        {
          ++index;
          std::cout
          << aec::cursor_right
          << std::flush;
        }
        break;
      }

      // ctrl-e
      if (static_cast<int>(c) == ctrl_key('e'))
      {
        // move cursor to end of line
        if (index < input.size())
        {
          index = input.size();
          std::cout
          << aec::cursor_set(index + 2, height)
          << std::flush;
        }
        break;
      }

      // ctrl-a
      if (static_cast<int>(c) == ctrl_key('a'))
      {
        // move cursor to start of line
        if (index)
        {
          index = 0;
          std::cout
          << aec::cursor_set(index + 2, height)
          << std::flush;
        }
        break;
      }

      // enter
      if (c == '\n')
      {
        // submit the input string
        loop = false;
        break;
      }

      // tab
      if (c == '\t')
      {
        break;
      }

      // backspace
      if (static_cast<int>(c) == 127 || static_cast<int>(c) == ctrl_key('h'))
      {
        // erase char behind cursor
        if (index)
        {
          input.erase(index - 1, 1);
          --index;

          std::cout
          << aec::cursor_hide
          << aec::cursor_save
          << aec::cr
          << aec::erase_line
          << prompt
          << input
          << aec::cursor_load
          << aec::cursor_left
          << aec::cursor_show
          << std::flush;
        }

        // exit the command prompt
        else if (input.empty())
        {
          loop = false;
          input.clear();
          break;
        }

        break;
      }

      if (input.size() + 2 >= width)
      {
        break;
      }

      // insert or append char to input buffer
      if (index < input.size())
      {
        input.insert(index, 1, c);
        ++index;
      }
      else
      {
        input += c;
        ++index;
      }

      // echo input buffer to stdout
      std::cout
      << aec::cursor_hide
      << aec::cursor_save
      << aec::cr
      << aec::erase_line
      << prompt
      << input
      << aec::cursor_load
      << aec::cursor_right
      << aec::cursor_show
      << std::flush;
    }

    if (num_read == 0)
    {
      std::this_thread::sleep_for(wait);
    }
  }

  return input;
}
