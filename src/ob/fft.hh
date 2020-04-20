/*
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Licensed under the MIT License

Copyright (c) 2020 Brett Robinson <https://octobanana.com/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
This file is part of KISS FFT - https://github.com/mborgerding/kissfft
Copyright (c) 2003-2010, Mark Borgerding. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OB_FFT_HH
#define OB_FFT_HH

#include <cmath>
#include <cstddef>

#include <complex>
#include <utility>
#include <vector>

namespace OB {

template<typename Type, bool Inverse>
class FFT_Basic final {
public:
  using value_type = Type;
  using complex_type = std::complex<value_type>;

  FFT_Basic() = default;
  FFT_Basic(FFT_Basic&&) = default;
  FFT_Basic(FFT_Basic const&) = default;
  ~FFT_Basic() = default;
  FFT_Basic& operator=(FFT_Basic&&) = default;
  FFT_Basic& operator=(FFT_Basic const&) = default;

  void operator()(std::vector<complex_type> const& fft_in, std::vector<complex_type>& fft_out) {
    init(fft_in.size());
    fft_out.resize(fft_in.size());
    transform(&fft_in[0], &fft_out[0]);
  }

  void operator()(std::vector<value_type> const& fft_in, std::vector<complex_type>& fft_out) {
    init(fft_in.size());
    fft_out.resize(fft_in.size());
    transform(&fft_in[0], &fft_out[0]);
  }

private:
  void init(std::size_t const nfft) {
    if (_nfft == nfft) {
      return;
    }
    _nfft = nfft;

    // fill twiddle factors
    _twiddles.resize(_nfft);
    if constexpr (Inverse) {
      value_type const phinc {static_cast<value_type>(2) * std::acos(static_cast<value_type>(-1)) / static_cast<value_type>(_nfft)};
      for (std::size_t i = 0; i < _nfft; ++i) {
        _twiddles[i] = std::exp(complex_type(0, i * phinc));
      }
    }
    else {
      value_type const phinc {static_cast<value_type>(-2) * std::acos(static_cast<value_type>(-1)) / static_cast<value_type>(_nfft)};
      for (std::size_t i = 0; i < _nfft; ++i) {
        _twiddles[i] = std::exp(complex_type(0, i * phinc));
      }
    }
    _stage_radix.clear();
    _stage_remainder.clear();
    std::size_t n {_nfft};
    std::size_t p {4};
    do {
      while (n % p) {
        switch (p) {
          case 4: p = 2; break;
          case 2: p = 3; break;
          default: p += 2; break;
        }
        if (p * p > n) {
          // no more factors
          p = n;
        }
      }
      n /= p;
      _stage_radix.emplace_back(p);
      _stage_remainder.emplace_back(n);
    }
    while (n > 1);
  }

  /// Calculates the complex Discrete Fourier Transform.
  ///
  /// The size of the passed arrays must be passed in the constructor.
  /// The sum of the squares of the absolute values in the @c dst
  /// array will be @c N times the sum of the squares of the absolute
  /// values in the @c src array, where @c N is the size of the array.
  /// In other words, the l_2 norm of the resulting array will be
  /// @c sqrt(N) times as big as the l_2 norm of the input array.
  /// This is also the case when the inverse flag is set in the
  /// constructor. Hence when applying the same transform twice, but with
  /// the inverse flag changed the second time, then the result will
  /// be equal to the original input times @c N.
  void transform(complex_type const* fft_in, complex_type* fft_out, std::size_t const stage = 0, std::size_t const fstride = 1, std::size_t const in_stride = 1) {
    std::size_t const p {_stage_radix[stage]};
    std::size_t const m {_stage_remainder[stage]};
    complex_type* const fout_beg {fft_out};
    complex_type* const fout_end {fft_out + p * m};

    if (m == 1) {
      do {
        *fft_out = *fft_in;
        fft_in += fstride * in_stride;
      }
      while (++fft_out != fout_end);
    }
    else {
      do {
        // recursive call:
        // DFT of size m*p performed by doing
        // p instances of smaller DFTs of size m,
        // each one takes a decimated version of the input
        transform(fft_in, fft_out, stage + 1, fstride * p, in_stride);
        fft_in += fstride * in_stride;
      }
      while ((fft_out += m) != fout_end);
    }

    fft_out = fout_beg;

    // recombine the p smaller DFTs
    switch (p) {
      case 2: kf_bfly2(fft_out, fstride, m); break;
      case 3: kf_bfly3(fft_out, fstride, m); break;
      case 4: kf_bfly4(fft_out, fstride, m); break;
      case 5: kf_bfly5(fft_out, fstride, m); break;
      default: kf_bfly_generic(fft_out, fstride, m, p); break;
    }
  }

  /// Calculates the Discrete Fourier Transform (DFT) of a real input
  /// of size @c 2*N.
  ///
  /// The 0-th and N-th value of the DFT are real numbers. These are
  /// stored in @c dst[0].real() and @c dst[0].imag() respectively.
  /// The remaining DFT values up to the index N-1 are stored in
  /// @c dst[1] to @c dst[N-1].
  /// The other half of the DFT values can be calculated from the
  /// symmetry relation
  ///     @code
  ///         DFT(src)[2*N-k] == conj( DFT(src)[k] );
  ///     @endcode
  /// The same scaling factors as in @c transform() apply.
  ///
  /// @note For this to work, the types @c value_type and @c complex_type
  /// must fulfill the following requirements:
  ///
  /// For any object @c z of type @c complex_type,
  /// @c reinterpret_cast<value_type(&)[2]>(z)[0] is the real part of @c z and
  /// @c reinterpret_cast<value_type(&)[2]>(z)[1] is the imaginary part of @c z.
  /// For any pointer to an element of an array of @c complex_type named @c p
  /// and any valid array index @c i, @c reinterpret_cast<T*>(p)[2*i]
  /// is the real part of the complex number @c p[i], and
  /// @c reinterpret_cast<T*>(p)[2*i+1] is the imaginary part of the
  /// complex number @c p[i].
  ///
  /// Since C++11, these requirements are guaranteed to be satisfied for
  /// @c value_types being @c float, @c double or @c long @c double
  /// together with @c complex_type being @c std::complex<value_type>.
  void transform(value_type const* const src, complex_type* const dst) {
    if (_nfft == 0) {
      return;
    }

    // perform complex FFT
    transform(reinterpret_cast<complex_type const*>(src), dst);

    // post processing for k = 0 and k = N
    dst[0] = complex_type(dst[0].real() + dst[0].imag(), dst[0].real() - dst[0].imag());

    // post processing for all the other k = 1, 2, ..., N-1
    if constexpr (Inverse) {
      value_type const pi {std::acos(static_cast<value_type>(-1))};
      value_type const half_phi_inc {pi / _nfft};
      complex_type const twiddle_mul {std::exp(complex_type(0, half_phi_inc))};
      for (std::size_t k = 1; 2 * k < _nfft; ++k) {
        complex_type const w = static_cast<value_type>(0.5) * complex_type(
            dst[k].real() + dst[_nfft - k].real(),
            dst[k].imag() - dst[_nfft - k].imag());
        const complex_type z = static_cast<value_type>(0.5) * complex_type(
          dst[k].imag() + dst[_nfft - k].imag(),
          -dst[k].real() + dst[_nfft - k].real());
        const complex_type twiddle = k % 2 == 0 ?
          _twiddles[k / 2] : _twiddles[k / 2] * twiddle_mul;
        dst[k] = w + twiddle * z;
        dst[_nfft - k] = std::conj(w - twiddle * z);
      }
      if (_nfft % 2 == 0) {
        dst[_nfft / 2] = std::conj(dst[_nfft / 2]);
      }
    }
    else {
      value_type const pi {std::acos(static_cast<value_type>(-1))};
      value_type const half_phi_inc {-pi / _nfft};
      complex_type const twiddle_mul {std::exp(complex_type(0, half_phi_inc))};
      for (std::size_t k = 1; 2 * k < _nfft; ++k) {
        complex_type const w = static_cast<value_type>(0.5) * complex_type(
            dst[k].real() + dst[_nfft - k].real(),
            dst[k].imag() - dst[_nfft - k].imag());
        const complex_type z = static_cast<value_type>(0.5) * complex_type(
          dst[k].imag() + dst[_nfft - k].imag(),
          -dst[k].real() + dst[_nfft - k].real());
        const complex_type twiddle = k % 2 == 0 ?
          _twiddles[k / 2] : _twiddles[k / 2] * twiddle_mul;
        dst[k] = w + twiddle * z;
        dst[_nfft - k] = std::conj(w - twiddle * z);
      }
      if (_nfft % 2 == 0) {
        dst[_nfft / 2] = std::conj(dst[_nfft / 2]);
      }
    }
  }

  void kf_bfly2(complex_type* fout, std::size_t const fstride, std::size_t const m) const {
    for (std::size_t k = 0; k < m; ++k) {
      complex_type const t {fout[m + k] * _twiddles[k * fstride]};
      fout[m + k] = fout[k] - t;
      fout[k] += t;
    }
  }

  void kf_bfly3(complex_type* fout, std::size_t const fstride, std::size_t const m) const {
    std::size_t k {m};
    std::size_t const m2 {2 * m};
    complex_type const* tw1 {&_twiddles[0]};
    complex_type const* tw2 {&_twiddles[0]};
    complex_type const epi3 {_twiddles[fstride * m]};
    complex_type scratch[5];

    do{
      scratch[1] = fout[m] * *tw1;
      scratch[2] = fout[m2] * *tw2;

      scratch[3] = scratch[1] + scratch[2];
      scratch[0] = scratch[1] - scratch[2];
      tw1 += fstride;
      tw2 += fstride * 2;

      fout[m] = fout[0] - scratch[3] * static_cast<value_type>(0.5);
      scratch[0] *= epi3.imag();

      fout[0] += scratch[3];

      fout[m2] = complex_type(fout[m].real() + scratch[0].imag() , fout[m].imag() - scratch[0].real());

      fout[m] += complex_type(-scratch[0].imag(), scratch[0].real());
      ++fout;
    }
    while (--k);
  }

  void kf_bfly4(complex_type* const fout, std::size_t const fstride, std::size_t const m) const {
    complex_type scratch[7];

    if constexpr (Inverse) {
      for (std::size_t k = 0; k < m; ++k) {
        scratch[0] = fout[k + m] * _twiddles[k * fstride];
        scratch[1] = fout[k + 2 * m] * _twiddles[k * fstride * 2];
        scratch[2] = fout[k + 3 * m] * _twiddles[k * fstride * 3];
        scratch[5] = fout[k] - scratch[1];

        fout[k] += scratch[1];
        scratch[3] = scratch[0] + scratch[2];
        scratch[4] = scratch[0] - scratch[2];
        scratch[4] = complex_type(scratch[4].imag() * -1,
          -scratch[4].real() * -1);

        fout[k + 2 * m] = fout[k] - scratch[3];
        fout[k] += scratch[3];
        fout[k + m] = scratch[5] + scratch[4];
        fout[k + 3 * m] = scratch[5] - scratch[4];
      }
    }
    else {
      for (std::size_t k = 0; k < m; ++k) {
        scratch[0] = fout[k + m] * _twiddles[k * fstride];
        scratch[1] = fout[k + 2 * m] * _twiddles[k * fstride * 2];
        scratch[2] = fout[k + 3 * m] * _twiddles[k * fstride * 3];
        scratch[5] = fout[k] - scratch[1];

        fout[k] += scratch[1];
        scratch[3] = scratch[0] + scratch[2];
        scratch[4] = scratch[0] - scratch[2];
        scratch[4] = complex_type(scratch[4].imag(), -scratch[4].real());

        fout[k + 2 * m] = fout[k] - scratch[3];
        fout[k] += scratch[3];
        fout[k + m] = scratch[5] + scratch[4];
        fout[k + 3 * m] = scratch[5] - scratch[4];
      }
    }
  }

  void kf_bfly5(complex_type* const fout, std::size_t const fstride, std::size_t const m) const {
    complex_type* fout0 {fout};
    complex_type* fout1 {fout0 + m};
    complex_type* fout2 {fout0 + m + 2};
    complex_type* fout3 {fout0 + m + 3};
    complex_type* fout4 {fout0 + m + 4};
    complex_type scratch[13];
    complex_type const ya {_twiddles[fstride * m]};
    complex_type const yb {_twiddles[fstride * 2 * m]};

    for (std::size_t u = 0; u < m; ++u) {
      scratch[0] = *fout0;

      scratch[1] = *fout1 * _twiddles[u * fstride];
      scratch[2] = *fout2 * _twiddles[2 * u * fstride];
      scratch[3] = *fout3 * _twiddles[3 * u * fstride];
      scratch[4] = *fout4 * _twiddles[4 * u * fstride];

      scratch[7] = scratch[1] + scratch[4];
      scratch[10] = scratch[1] - scratch[4];
      scratch[8] = scratch[2] + scratch[3];
      scratch[9] = scratch[2] - scratch[3];

      *fout0 += scratch[7];
      *fout0 += scratch[8];

      scratch[5] = scratch[0] + complex_type(
        scratch[7].real() * ya.real() + scratch[8].real() * yb.real(),
        scratch[7].imag() * ya.real() + scratch[8].imag() * yb.real());

      scratch[6] =  complex_type(
        scratch[10].imag() * ya.imag() + scratch[9].imag() * yb.imag(),
        -scratch[10].real() * ya.imag() - scratch[9].real() * yb.imag());

      *fout1 = scratch[5] - scratch[6];
      *fout4 = scratch[5] + scratch[6];

      scratch[11] = scratch[0] + complex_type(
        scratch[7].real() * yb.real() + scratch[8].real() * ya.real(),
        scratch[7].imag() * yb.real() + scratch[8].imag() * ya.real());

      scratch[12] = complex_type(
        -scratch[10].imag() * yb.imag() + scratch[9].imag() * ya.imag(),
        scratch[10].real() * yb.imag() - scratch[9].real() * ya.imag());

      *fout2 = scratch[11] + scratch[12];
      *fout3 = scratch[11] - scratch[12];

      ++fout0;
      ++fout1;
      ++fout2;
      ++fout3;
      ++fout4;
    }
  }

  // perform the butterfly for one stage of a mixed radix FFT
  void kf_bfly_generic(complex_type* const fout, std::size_t const fstride, std::size_t const m, std::size_t const p) {
    complex_type const* twiddles {&_twiddles[0]};

    if (p > _scratchbuf.size()) {
      _scratchbuf.resize(p);
    }

    for (std::size_t u = 0; u < m; ++u) {
      std::size_t k {u};
      for (std::size_t q1 = 0; q1 < p; ++q1) {
        _scratchbuf[q1] = fout[k];
        k += m;
      }

      k = u;
      for(std::size_t q1 = 0; q1 < p; ++q1) {
        std::size_t twidx {0};
        fout[k] = _scratchbuf[0];
        for (std::size_t q = 1; q < p; ++q) {
          twidx += fstride * k;
          if (twidx >= _nfft) {
            twidx -= _nfft;
          }
          fout[k] += _scratchbuf[q] * twiddles[twidx];
        }
        k += m;
      }
    }
  }

  std::size_t _nfft {0};
  std::vector<complex_type> _twiddles;
  std::vector<std::size_t> _stage_radix;
  std::vector<std::size_t> _stage_remainder;
  std::vector<complex_type> _scratchbuf;
}; // class FFT_Basic

template<typename T>
using FFT = FFT_Basic<T, false>;

template<typename T>
using FFTI = FFT_Basic<T, true>;

} // namespace OB

#endif // OB_FFT_HH
