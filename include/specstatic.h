/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef SPECSTATIC_H
#define SPECSTATIC_H

#include <limits.h>
#include <stdint.h>
#include <cstddef>
#include <string>
#include <utility>
#include <array>

/* SpecNet const: */
#define  GUID_LEN  19
#define  SMAX_GROUPS  100
#define  SMAX_PATH  300
#define  MIN_GUID  1000000000000000000
#define  MAX_SelectRows  50

//uint32                  4294967295
//max                     2147483647
#define SERV_TYPE  1923082018
#define CLI_TYPE    1924082018

/* default times in ms: */
#define  DEADLOCK_TIME  5000
static const int64_t  JOB_IDLE_TIME_ms  { 15000 };
#define  WAIT_TIME  100
constexpr  int64_t  DAY_MILLISEC  =  24  *  60  *  60  *  1000;

/* default array sizes in bytes: */
#define  EPOLL_WAIT_POOL  21
#define  BUF_SIZE  4096

//until about the month counter % 12:
#define  TO12(x)  ((((x)>>31)%12))
// months will increment, so new folders each month:
static int64_t DEF_time_group_factor = 30ll // days
                                            * 24ll * 60ll * 60ll * 1000ll;

/*  Converts string to number
  first  -  poiner to the first character
  len - expected length, its OK if string ends earlier
 */
inline  int64_t  stoll(const char  *first,  int  len)  {
  if (len  >  GUID_LEN)  {
    len  =  GUID_LEN;
  }
  const char  *last  =  first  +  len;
  uint64_t  re  = 0ULL; //  18 446 744 073 709 551 615 = 20
  for (const char *ch  =  first;  *ch  &&  ch<last;  ++ch)  {
    if  (*ch<'0'  ||  *ch>'9')  {  break;  }
      re  =  10 * re  +  *ch  -  '0';
    }
    return  (re  >  LLONG_MAX)?  LLONG_MAX  :  re;
}

/* Prints chars to the string buffer.
  Used for quickly build strings.
  str - chars to be printed
  start - string buffer
  end - end of the string buffer, must be valid for change
*/
inline char  *  printString(const char  *str,  char  *start,  char  *end){
  while  (*str  &&  start < end)  {
    *start  =  *str;
    ++start;
    ++str;
  }
    //if (start<end) { *start =0; }
  *start  =  0;  //  *end must be valid for change
  return start;
}

/* Prints uint64_t to the string buffer.
  Used for quickly build strings.
  n - uint64_t to be printed
  start - string buffer
  end - end of the string buffer, must be valid for change
*/
inline char  *  printULong(uint64_t  n,  char  *start,  char  *end)  {
  if (0==n)  {
    *start ='0';
    ++start;
  }  else  {
    char  buf[24];
    char  *ch  =  buf;
    uint64_t  n1  =  n;
    while  (0!=n1)  {
      *ch  =  n1%10  +  '0';
      ++ch;
      n1  =  n1/10;
    }
    --ch;
    while  (ch>=buf  &&  start<end)  {
      *start  =  *ch;
      ++start;
      --ch;
    }
  }
    //if (start<end) { *start =0; }
  *start  =  0;  //  *end must be valid for change
  return start;
}

static inline long long flagOn(long long masterFlags, int flagPos) {
    masterFlags = masterFlags | (1LL << flagPos);
    return masterFlags;
}

static inline uint32_t flagOnI(uint32_t masterFlags, int flagPos) {
    masterFlags = masterFlags | (1U << flagPos);
    return masterFlags;
}

static inline long long flagOff(long long masterFlags, int flagPos) {
    masterFlags = masterFlags & ~(1LL << flagPos);
    return masterFlags;
}

static inline uint32_t flagOffI(uint32_t masterFlags, int flagPos) {
    masterFlags = masterFlags & ~(1U << flagPos);
    return masterFlags;
}

static inline long long flagSet(long long masterFlags, int flagPos, bool is) {
    if (is) {
        masterFlags = masterFlags | (1LL << flagPos);
    } else {
        masterFlags = masterFlags & ~(1LL << flagPos);
    }
    return masterFlags;
}


static inline uint32_t flagSetI(uint32_t masterFlags, int flagPos, bool is) {
    if (is) {
        masterFlags = masterFlags | (1U << flagPos);
    } else {
        masterFlags = masterFlags & ~(1U << flagPos);
    }
    return masterFlags;
}

static inline bool getFlagI(uint32_t masterFlags, int flagPos) {
    return 1U==((masterFlags >> flagPos) & 1U);
}

static inline bool getFlag(long long masterFlags, int flagPos) {
    return 1LL==((masterFlags >> flagPos) & 1LL);
}

/* Constexpr exception free string class.
  Used for static const strings.
*/
class  ConstString  {
 public:
  template<std::size_t N>
  constexpr ConstString(const char (&str)[N])
    :  c_str(str),  size(N - 1) {
  }

  // Warn: use it only if sure str exist while ConstString in use:
  ConstString(const char *str, unsigned int  len)
      :  c_str(str),  size(len) {
    }

  ConstString(std::string &str)
      :  c_str(str.c_str()),  size(static_cast<uint32_t>(str.length())) {
    }

  constexpr char operator[](std::size_t  n) {
    return  (n < size)?  c_str[n]  :  '\0';
  }

  char const  *const  c_str;
  //const std::size_t size;
  //const int  size; //SQLite wants int
  const uint32_t  size;
};

///////////////////////////////////////////////////////////////////
/* consexpr unsigned char */
template<unsigned...>struct seq{using type=seq;};
template<unsigned N, unsigned... Is>
struct gen_seq_x : gen_seq_x<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq_x<0, Is...> : seq<Is...>{};
template<unsigned N>
using gen_seq=typename gen_seq_x<N>::type;

template<size_t S>
using size=std::integral_constant<size_t, S>;

template<class T, size_t N>
constexpr size<N> length( T const(&)[N] ) { return {}; }
template<class T, size_t N>
constexpr size<N> length( std::array<T, N> const& ) { return {}; }

template<class T>
using length_t = decltype(length(std::declval<T>()));

constexpr size_t string_size() { return 0; }
template<class...Ts>
constexpr size_t string_size( size_t i, Ts... ts ) {
  return (i?i-1:0) + string_size(ts...);
}
template<class...Ts>
using string_length=size< string_size( length_t<Ts>{}... )>;

template<class...Ts>
using combined_ustring = std::array<unsigned char, string_length<Ts...>{}+1>;

template<class Lhs, class Rhs, unsigned...I1, unsigned...I2>
constexpr const combined_ustring<Lhs,Rhs>
uconcat_impl( Lhs const& lhs, Rhs const& rhs, seq<I1...>, seq<I2...>)
{
    return {{ (unsigned char)lhs[I1]..., (unsigned char)rhs[I2]..., (unsigned char)('\0') }};
}

template<class Lhs, class Rhs>
constexpr const combined_ustring<Lhs,Rhs>
uconcat(Lhs const& lhs, Rhs const& rhs)
{
    return uconcat_impl(lhs, rhs, gen_seq<string_length<Lhs>{}>{}, gen_seq<string_length<Rhs>{}>{});
}

template<class T0, class T1, class... Ts>
constexpr const combined_ustring<T0, T1, Ts...>
uconcat(T0 const&t0, T1 const&t1, Ts const&...ts)
{
    return uconcat(t0, uconcat(t1, ts...));
}

template<class T>
constexpr const combined_ustring<T>
uconcat(T const&t) {
    return uconcat(t, "");
}
constexpr const combined_ustring<>
uconcat() {
    return uconcat("");
}


/*
 * usage:
 * template<typename T>
    class TD;
int main()
{
  {
    // works
    auto constexpr text = uconcat("hi", " ", "there!");
    std::cout << text.data();
    std::cout << text.size();
    TD<decltype(text)> xType;
  }
*/
///////////////////////////////////////////////////////////////////
/* consexpr char */
template<class...Ts>
using combined_string = std::array<char, string_length<Ts...>{}+1>;

template<class Lhs, class Rhs, unsigned...I1, unsigned...I2>
constexpr const combined_string<Lhs,Rhs>
concat_impl( Lhs const& lhs, Rhs const& rhs, seq<I1...>, seq<I2...>)
{
    return {{ lhs[I1]..., rhs[I2]..., '\0' }};
}

template<class Lhs, class Rhs>
constexpr const combined_string<Lhs,Rhs>
concat(Lhs const& lhs, Rhs const& rhs)
{
    return concat_impl(lhs, rhs, gen_seq<string_length<Lhs>{}>{}, gen_seq<string_length<Rhs>{}>{});
}

template<class T0, class T1, class... Ts>
constexpr const combined_string<T0, T1, Ts...>
concat(T0 const&t0, T1 const&t1, Ts const&...ts)
{
    return concat(t0, concat(t1, ts...));
}

template<class T>
constexpr const combined_string<T>
concat(T const&t) {
    return concat(t, "");
}
constexpr const combined_string<>
concat() {
    return concat("");
}

//template<typename A, typename B>
//constexpr auto
//test1(const A& a, const B& b)
//{
//  return concat(a, b);
//}
/*
 * usage:
 * template<typename T>
    class TD;
int main()
{
  {
    // works
    auto constexpr text = concat("hi", " ", "there!");
    std::cout << text.data();
    std::cout << text.size();
    TD<decltype(text)> xType;
  }
*/
///////////////////////////////////////////////////////////////////

#define CONCATE_(X,Y) X##Y
#define CONCATE(X,Y) CONCATE_(X,Y)
#define UNIQUE(NAME) CONCATE(NAME, __LINE__)

struct Static_
{
  template<typename T> Static_ (T lambda) { lambda(); }
  ~Static_ () {}  // to counter unused variable warning
};

#define STATIC static Static_ UNIQUE(block) = [&]() -> void

// Fast Bit operation
  constexpr std::uint_fast8_t mask0{ 0b00000001 }; // represents bit 0
  constexpr std::uint_fast8_t mask1{ 0b00000010 }; // represents bit 1
  constexpr std::uint_fast8_t mask2{ 0b00000100 }; // represents bit 2
  constexpr std::uint_fast8_t mask3{ 0b00001000 }; // represents bit 3
  constexpr std::uint_fast8_t mask4{ 0b00010000 }; // represents bit 4
  constexpr std::uint_fast8_t mask5{ 0b00100000 }; // represents bit 5
  constexpr std::uint_fast8_t mask6{ 0b01000000 }; // represents bit 6
  constexpr std::uint_fast8_t mask7{ 0b10000000 }; // represents bit 7
/*
 * https://www.learncpp.com/cpp-tutorial/bit-manipulation-with-bitwise-operators-and-bit-masks/
   Setting a bit
   std::uint_fast8_t flags{ 0b0000'0101 }; // 8 bits in size means room for 8 flags

    std::cout << "bit 1 is " << ((flags & mask1) ? "on\n" : "off\n");

    flags |= mask1; // turn on bit 1

    std::cout << "bit 1 is " << ((flags & mask1) ? "on\n" : "off\n");

    Resetting a bit
    std::uint_fast8_t flags{ 0b0000'0101 }; // 8 bits in size means room for 8 flags

    std::cout << "bit 2 is " << ((flags & mask2) ? "on\n" : "off\n");

    flags &= ~mask2; // turn off bit 2

    std::cout << "bit 2 is " << ((flags & mask2) ? "on\n" : "off\n");

    Flipping a bit
    std::uint_fast8_t flags{ 0b0000'0101 }; // 8 bits in size means room for 8 flags

    std::cout << "bit 2 is " << ((flags & mask2) ? "on\n" : "off\n");
    flags ^= mask2; // flip bit 2
    std::cout << "bit 2 is " << ((flags & mask2) ? "on\n" : "off\n");
    flags ^= mask2; // flip bit 2
    std::cout << "bit 2 is " << ((flags & mask2) ? "on\n" : "off\n");

    Bit masks and std::bitset
  std::bitset<8> flags{ 0b0000'0101 }; // 8 bits in size means room for 8 flags

  std::cout << "bit 1 is " << (flags.test(1) ? "on\n" : "off\n");
  std::cout << "bit 2 is " << (flags.test(2) ? "on\n" : "off\n");
  flags ^= (mask1 | mask2); // flip bits 1 and 2

  std::cout << "bit 1 is " << (flags.test(1) ? "on\n" : "off\n");
  std::cout << "bit 2 is " << (flags.test(2) ? "on\n" : "off\n");
  flags |= (mask1 | mask2); // turn bits 1 and 2 on

  std::cout << "bit 1 is " << (flags.test(1) ? "on\n" : "off\n");
  std::cout << "bit 2 is " << (flags.test(2) ? "on\n" : "off\n");
  flags &= ~(mask1 | mask2); // turn bits 1 and 2 off

  std::cout << "bit 1 is " << (flags.test(1) ? "on\n" : "off\n");
  std::cout << "bit 2 is " << (flags.test(2) ? "on\n" : "off\n");

  Making bit masks meaningful
  std::uint_fast8_t me{}; // all flags/options turned off to start
  me |= isHappy | isLaughing; // I am happy and laughing
  me &= ~isLaughing; // I am no longer laughing

  // Query a few states
  // (we'll use static_cast<bool> to interpret the results as a boolean value)
  std::cout << "I am happy? " << static_cast<bool>(me & isHappy) << '\n';
  std::cout << "I am laughing? " << static_cast<bool>(me & isLaughing) << '\n';

Here’s a sample function call from OpenGL:
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the color and the depth buffer
GL_COLOR_BUFFER_BIT and GL_DEPTH_BUFFER_BIT are bit masks defined as follows (in gl2.h):

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

Bit masks involving multiple bits
  constexpr std::uint_fast32_t redBits{ 0xFF000000 };
  constexpr std::uint_fast32_t greenBits{ 0x00FF0000 };
  constexpr std::uint_fast32_t blueBits{ 0x0000FF00 };
  constexpr std::uint_fast32_t alphaBits{ 0x000000FF };

  std::cout << "Enter a 32-bit RGBA color value in hexadecimal (e.g. FF7F3300): ";
  std::uint_fast32_t pixel{};
  std::cin >> std::hex >> pixel; // std::hex allows us to read in a hex value

  // use Bitwise AND to isolate red pixels,
  // then right shift the value into the lower 8 bits
  // (we're not using brace initialization to avoid a static_cast)
  std::uint_fast8_t red = (pixel & redBits) >> 24;
  std::uint_fast8_t green = (pixel & greenBits) >> 16;
  std::uint_fast8_t blue = (pixel & blueBits) >> 8;
  std::uint_fast8_t alpha = pixel & alphaBits;

  std::cout << "Your color contains:\n";
  std::cout << std::hex; // print the following values in hex
  std::cout << static_cast<int>(red) << " red\n";
  std::cout << static_cast<int>(green) << " green\n";
  std::cout << static_cast<int>(blue) << " blue\n";
  std::cout << static_cast<int>(alpha) << " alpha\n";

  Summary
Summarizing how to set, clear, toggle, and query bit flags:

To query bit states, we use bitwise AND:

1
if (flags & option4) ... // if option4 is set, do something
To set bits (turn on), we use bitwise OR:

1
2
flags |= option4; // turn option 4 on.
flags |= (option4 | option5); // turn options 4 and 5 on.
To clear bits (turn off), we use bitwise AND with bitwise NOT:

1
2
flags &= ~option4; // turn option 4 off
flags &= ~(option4 | option5); // turn options 4 and 5 off
To flip bit states, we use bitwise XOR:

1
2
flags ^= option4; // flip option4 from on to off, or vice versa
flags ^= (option4 | option5); // flip options 4 and 5
*/
#endif // SPECSTATIC_H
