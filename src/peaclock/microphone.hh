#ifndef MICROPHONE_HH
#define MICROPHONE_HH

#include "ob/fft.hh"

#include <SFML/Audio.hpp>

#include <cmath>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <vector>
#include <complex>

class Microphone : public sf::SoundRecorder {
public:
  struct Band {
    double min {0};
    double max {0};
    double val {0};
  };

  struct Bands {
    Band sub_bass {20, 60, 0};
    Band bass {60, 250, 0};
    Band low_midrange {250, 500, 0};
    Band midrange {500, 2000, 0};
    Band upper_midrange {2000, 4000, 0};
    Band presence {4000, 6000, 0};
    Band brilliance {6000, 20000, 0};
  };

  Microphone() {
    _samples.resize(_size);
    _inbuf.resize(_size);
    _outbuf.resize(_size);
    _fmtbuf.reserve(_size);
    _hann.reserve(_size);
    for (std::size_t i = 0; i < _size; ++i) {
      _hann.emplace_back(0.54 * (1.0 - std::cos(2.0 * M_PI * i / static_cast<double>(_size))));
    }
  }

  ~Microphone() {
  }

  void setProcessingInterval(sf::Time interval) {
    sf::SoundRecorder::setProcessingInterval(interval);
  }

  bool isSpeaking() const {
    return _speaking;
  }

  Bands const& getBands() const {
    return _bands;
  }

  std::vector<double> const& getBuffer() const {
    return _fmtbuf;
  }

  std::vector<double> const& getSamples() const {
    return _samples;
  }

  std::size_t size() const {
    return _inbuf.size();
  }

private:
  using value_type = double;
  using FFT = OB::FFT<value_type>;
  using complex_type = std::complex<value_type>;

  bool onStart() {
    // std::cerr << "Rec> Start\n\n";
    return true;
  }

  void onStop() {
    // std::cerr << "\nRec> Stop\n";
  }

  bool onProcessSamples(sf::Int16 const* samples, std::size_t size) {
    if (!size || size < _size) {return true;}
    size = _size;

    // apply scaling and window function
    // set imaginary part of complex number to zero
    double scale_pos {1.0 / 32767.0};
    double scale_neg {1.0 / 32768.0};
    for (std::size_t i = 0; i < size; ++i) {
      _samples[i] = (samples[i] > 0 ? samples[i] * scale_pos : samples[i] * scale_neg);
      _inbuf[i] = complex_type((samples[i] > 0 ? samples[i] * scale_pos : samples[i] * scale_neg) * _hann[i], 0);
    }

    // apply fast fourier transform
    _fft(_inbuf, _outbuf);

    auto bin = getSampleRate() / size;

    // only deal with the positive frequency values
    // [dc, +f, n/2 (nyquist freq), -f, n]
    _outbuf.erase(_outbuf.begin() + static_cast<long int>(_outbuf.size() / 2), _outbuf.end());
    size = _outbuf.size();

    // format data into decibels
    _fmtbuf.clear();
    for (std::size_t i = 0; i < _outbuf.size(); ++i) {
      auto const& val = _outbuf[i];
      auto const magnitude = 2.0 * std::sqrt(val.real() * val.real() + val.imag() * val.imag()) / static_cast<double>(size);
      auto const decibels = 20.0 * std::log10(magnitude);
      _fmtbuf.emplace_back(decibels);
    }

    // calculate decibel bands
    {
      auto const calc_band = [&](auto& band) {
        band.val = 0;
        std::size_t const begin = band.min / bin;
        std::size_t const end = (band.max / bin) + 1;
        for (std::size_t i = begin; i < end && i < _fmtbuf.size(); ++i) {
          band.val += _fmtbuf[i];
        }
        band.val = band.val / (end - begin);
      };

      calc_band(_bands.sub_bass);
      calc_band(_bands.bass);
      calc_band(_bands.low_midrange);
      calc_band(_bands.midrange);
      calc_band(_bands.upper_midrange);
      calc_band(_bands.presence);
      calc_band(_bands.brilliance);
    }

    // apply low pass filter
    _fmtbuf.erase(_fmtbuf.begin() + static_cast<long int>(low_pass / bin), _fmtbuf.end());

    // apply high pass filter
    _fmtbuf.erase(_fmtbuf.begin(), _fmtbuf.begin() + static_cast<long int>(high_pass / bin));

    return true;
  }

  std::size_t _size {1024};
  std::size_t low_pass {20000};
  std::size_t high_pass {20};
  bool _speaking {false};
  FFT _fft;
  std::vector<value_type> _samples;
  std::vector<complex_type> _inbuf;
  std::vector<complex_type> _outbuf;
  std::vector<value_type> _fmtbuf;
  std::vector<value_type> _hann;

  Bands _bands;
};

#endif // MICROPHONE_HH
