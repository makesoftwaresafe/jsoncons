/*
Implements the Grisu3 algorithm for printing floating-point numbers. 
 
Follows Florian Loitsch's grisu3_59_56 implementation, available at
http://florian.loitsch.com/publications, in bench.tar.gz, with 
minor modifications. 
*/

/*
  Copyright (c) 2009 Florian Loitsch

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without
  restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following
  conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.
  */

#ifndef JSONCONS_DETAIL_GRISU3_HPP
#define JSONCONS_DETAIL_GRISU3_HPP 

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstring> // std::memmove
#include <stdlib.h>

namespace jsoncons { 
namespace detail {

// diy_fp

typedef struct diy_fp_t {
    uint64_t f;
    int e;
} diy_fp_t;

inline 
diy_fp_t minus(diy_fp_t x, diy_fp_t y)
{
    assert(x.e == y.e);
    assert(x.f >= y.f);
    diy_fp_t r = { x.f = x.f - y.f, x.e };
    return r;
}

inline 
diy_fp_t multiply(diy_fp_t x, diy_fp_t y)
{
    uint64_t a, b, c, d, ac, bc, ad, bd, tmp;
    diy_fp_t r; uint64_t M32 = 0xFFFFFFFF;
    a = x.f >> 32; b = x.f & M32;
    c = y.f >> 32; d = y.f & M32;
    ac = a * c; bc = b * c; ad = a * d; bd = b * d;
    tmp = (bd >> 32) + (ad & M32) + (bc & M32);
    tmp += 1U << 31; /// mult_round
    r.f = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
    r.e = x.e + y.e + 64;
    return r;
}

// k_comp

inline 
int k_comp(int e, int alpha, int /*gamma*/)
{
    constexpr double d_1_log2_10 = 0.30102999566398114; //  1 / lg(10)
    int x = alpha - e + 63;
    return static_cast<int>(std::ceil(x * d_1_log2_10));
}

// powers_ten_round64

constexpr int diy_significand_size = 64;

static constexpr uint64_t powers_ten[] = { 0xbf29dcaba82fdeae, 0xeef453d6923bd65a, 0x9558b4661b6565f8, 0xbaaee17fa23ebf76, 0xe95a99df8ace6f54, 0x91d8a02bb6c10594, 0xb64ec836a47146fa, 0xe3e27a444d8d98b8, 0x8e6d8c6ab0787f73, 0xb208ef855c969f50, 0xde8b2b66b3bc4724, 0x8b16fb203055ac76, 0xaddcb9e83c6b1794, 0xd953e8624b85dd79, 0x87d4713d6f33aa6c, 0xa9c98d8ccb009506, 0xd43bf0effdc0ba48, 0x84a57695fe98746d, 0xa5ced43b7e3e9188, 0xcf42894a5dce35ea, 0x818995ce7aa0e1b2, 0xa1ebfb4219491a1f, 0xca66fa129f9b60a7, 0xfd00b897478238d1, 0x9e20735e8cb16382, 0xc5a890362fddbc63, 0xf712b443bbd52b7c, 0x9a6bb0aa55653b2d, 0xc1069cd4eabe89f9, 0xf148440a256e2c77, 0x96cd2a865764dbca, 0xbc807527ed3e12bd, 0xeba09271e88d976c, 0x93445b8731587ea3, 0xb8157268fdae9e4c, 0xe61acf033d1a45df, 0x8fd0c16206306bac, 0xb3c4f1ba87bc8697, 0xe0b62e2929aba83c, 0x8c71dcd9ba0b4926, 0xaf8e5410288e1b6f, 0xdb71e91432b1a24b, 0x892731ac9faf056f, 0xab70fe17c79ac6ca, 0xd64d3d9db981787d, 0x85f0468293f0eb4e, 0xa76c582338ed2622, 0xd1476e2c07286faa, 0x82cca4db847945ca, 0xa37fce126597973d, 0xcc5fc196fefd7d0c, 0xff77b1fcbebcdc4f, 0x9faacf3df73609b1, 0xc795830d75038c1e, 0xf97ae3d0d2446f25, 0x9becce62836ac577, 0xc2e801fb244576d5, 0xf3a20279ed56d48a, 0x9845418c345644d7, 0xbe5691ef416bd60c, 0xedec366b11c6cb8f, 0x94b3a202eb1c3f39, 0xb9e08a83a5e34f08, 0xe858ad248f5c22ca, 0x91376c36d99995be, 0xb58547448ffffb2e, 0xe2e69915b3fff9f9, 0x8dd01fad907ffc3c, 0xb1442798f49ffb4b, 0xdd95317f31c7fa1d, 0x8a7d3eef7f1cfc52, 0xad1c8eab5ee43b67, 0xd863b256369d4a41, 0x873e4f75e2224e68, 0xa90de3535aaae202, 0xd3515c2831559a83, 0x8412d9991ed58092, 0xa5178fff668ae0b6, 0xce5d73ff402d98e4, 0x80fa687f881c7f8e, 0xa139029f6a239f72, 0xc987434744ac874f, 0xfbe9141915d7a922, 0x9d71ac8fada6c9b5, 0xc4ce17b399107c23, 0xf6019da07f549b2b, 0x99c102844f94e0fb, 0xc0314325637a193a, 0xf03d93eebc589f88, 0x96267c7535b763b5, 0xbbb01b9283253ca3, 0xea9c227723ee8bcb, 0x92a1958a7675175f, 0xb749faed14125d37, 0xe51c79a85916f485, 0x8f31cc0937ae58d3, 0xb2fe3f0b8599ef08, 0xdfbdcece67006ac9, 0x8bd6a141006042be, 0xaecc49914078536d, 0xda7f5bf590966849, 0x888f99797a5e012d, 0xaab37fd7d8f58179, 0xd5605fcdcf32e1d7, 0x855c3be0a17fcd26, 0xa6b34ad8c9dfc070, 0xd0601d8efc57b08c, 0x823c12795db6ce57, 0xa2cb1717b52481ed, 0xcb7ddcdda26da269, 0xfe5d54150b090b03, 0x9efa548d26e5a6e2, 0xc6b8e9b0709f109a, 0xf867241c8cc6d4c1, 0x9b407691d7fc44f8, 0xc21094364dfb5637, 0xf294b943e17a2bc4, 0x979cf3ca6cec5b5b, 0xbd8430bd08277231, 0xece53cec4a314ebe, 0x940f4613ae5ed137, 0xb913179899f68584, 0xe757dd7ec07426e5, 0x9096ea6f3848984f, 0xb4bca50b065abe63, 0xe1ebce4dc7f16dfc, 0x8d3360f09cf6e4bd, 0xb080392cc4349ded, 0xdca04777f541c568, 0x89e42caaf9491b61, 0xac5d37d5b79b6239, 0xd77485cb25823ac7, 0x86a8d39ef77164bd, 0xa8530886b54dbdec, 0xd267caa862a12d67, 0x8380dea93da4bc60, 0xa46116538d0deb78, 0xcd795be870516656, 0x806bd9714632dff6, 0xa086cfcd97bf97f4, 0xc8a883c0fdaf7df0, 0xfad2a4b13d1b5d6c, 0x9cc3a6eec6311a64, 0xc3f490aa77bd60fd, 0xf4f1b4d515acb93c, 0x991711052d8bf3c5, 0xbf5cd54678eef0b7, 0xef340a98172aace5, 0x9580869f0e7aac0f, 0xbae0a846d2195713, 0xe998d258869facd7, 0x91ff83775423cc06, 0xb67f6455292cbf08, 0xe41f3d6a7377eeca, 0x8e938662882af53e, 0xb23867fb2a35b28e, 0xdec681f9f4c31f31, 0x8b3c113c38f9f37f, 0xae0b158b4738705f, 0xd98ddaee19068c76, 0x87f8a8d4cfa417ca, 0xa9f6d30a038d1dbc, 0xd47487cc8470652b, 0x84c8d4dfd2c63f3b, 0xa5fb0a17c777cf0a, 0xcf79cc9db955c2cc, 0x81ac1fe293d599c0, 0xa21727db38cb0030, 0xca9cf1d206fdc03c, 0xfd442e4688bd304b, 0x9e4a9cec15763e2f, 0xc5dd44271ad3cdba, 0xf7549530e188c129, 0x9a94dd3e8cf578ba, 0xc13a148e3032d6e8, 0xf18899b1bc3f8ca2, 0x96f5600f15a7b7e5, 0xbcb2b812db11a5de, 0xebdf661791d60f56, 0x936b9fcebb25c996, 0xb84687c269ef3bfb, 0xe65829b3046b0afa, 0x8ff71a0fe2c2e6dc, 0xb3f4e093db73a093, 0xe0f218b8d25088b8, 0x8c974f7383725573, 0xafbd2350644eead0, 0xdbac6c247d62a584, 0x894bc396ce5da772, 0xab9eb47c81f5114f, 0xd686619ba27255a3, 0x8613fd0145877586, 0xa798fc4196e952e7, 0xd17f3b51fca3a7a1, 0x82ef85133de648c5, 0xa3ab66580d5fdaf6, 0xcc963fee10b7d1b3, 0xffbbcfe994e5c620, 0x9fd561f1fd0f9bd4, 0xc7caba6e7c5382c9, 0xf9bd690a1b68637b, 0x9c1661a651213e2d, 0xc31bfa0fe5698db8, 0xf3e2f893dec3f126, 0x986ddb5c6b3a76b8, 0xbe89523386091466, 0xee2ba6c0678b597f, 0x94db483840b717f0, 0xba121a4650e4ddec, 0xe896a0d7e51e1566, 0x915e2486ef32cd60, 0xb5b5ada8aaff80b8, 0xe3231912d5bf60e6, 0x8df5efabc5979c90, 0xb1736b96b6fd83b4, 0xddd0467c64bce4a1, 0x8aa22c0dbef60ee4, 0xad4ab7112eb3929e, 0xd89d64d57a607745, 0x87625f056c7c4a8b, 0xa93af6c6c79b5d2e, 0xd389b47879823479, 0x843610cb4bf160cc, 0xa54394fe1eedb8ff, 0xce947a3da6a9273e, 0x811ccc668829b887, 0xa163ff802a3426a9, 0xc9bcff6034c13053, 0xfc2c3f3841f17c68, 0x9d9ba7832936edc1, 0xc5029163f384a931, 0xf64335bcf065d37d, 0x99ea0196163fa42e, 0xc06481fb9bcf8d3a, 0xf07da27a82c37088, 0x964e858c91ba2655, 0xbbe226efb628afeb, 0xeadab0aba3b2dbe5, 0x92c8ae6b464fc96f, 0xb77ada0617e3bbcb, 0xe55990879ddcaabe, 0x8f57fa54c2a9eab7, 0xb32df8e9f3546564, 0xdff9772470297ebd, 0x8bfbea76c619ef36, 0xaefae51477a06b04, 0xdab99e59958885c5, 0x88b402f7fd75539b, 0xaae103b5fcd2a882, 0xd59944a37c0752a2, 0x857fcae62d8493a5, 0xa6dfbd9fb8e5b88f, 0xd097ad07a71f26b2, 0x825ecc24c8737830, 0xa2f67f2dfa90563b, 0xcbb41ef979346bca, 0xfea126b7d78186bd, 0x9f24b832e6b0f436, 0xc6ede63fa05d3144, 0xf8a95fcf88747d94, 0x9b69dbe1b548ce7d, 0xc24452da229b021c, 0xf2d56790ab41c2a3, 0x97c560ba6b0919a6, 0xbdb6b8e905cb600f, 0xed246723473e3813, 0x9436c0760c86e30c, 0xb94470938fa89bcf, 0xe7958cb87392c2c3, 0x90bd77f3483bb9ba, 0xb4ecd5f01a4aa828, 0xe2280b6c20dd5232, 0x8d590723948a535f, 0xb0af48ec79ace837, 0xdcdb1b2798182245, 0x8a08f0f8bf0f156b, 0xac8b2d36eed2dac6, 0xd7adf884aa879177, 0x86ccbb52ea94baeb, 0xa87fea27a539e9a5, 0xd29fe4b18e88640f, 0x83a3eeeef9153e89, 0xa48ceaaab75a8e2b, 0xcdb02555653131b6, 0x808e17555f3ebf12, 0xa0b19d2ab70e6ed6, 0xc8de047564d20a8c, 0xfb158592be068d2f, 0x9ced737bb6c4183d, 0xc428d05aa4751e4d, 0xf53304714d9265e0, 0x993fe2c6d07b7fac, 0xbf8fdb78849a5f97, 0xef73d256a5c0f77d, 0x95a8637627989aae, 0xbb127c53b17ec159, 0xe9d71b689dde71b0, 0x9226712162ab070e, 0xb6b00d69bb55c8d1, 0xe45c10c42a2b3b06, 0x8eb98a7a9a5b04e3, 0xb267ed1940f1c61c, 0xdf01e85f912e37a3, 0x8b61313bbabce2c6, 0xae397d8aa96c1b78, 0xd9c7dced53c72256, 0x881cea14545c7575, 0xaa242499697392d3, 0xd4ad2dbfc3d07788, 0x84ec3c97da624ab5, 0xa6274bbdd0fadd62, 0xcfb11ead453994ba, 0x81ceb32c4b43fcf5, 0xa2425ff75e14fc32, 0xcad2f7f5359a3b3e, 0xfd87b5f28300ca0e, 0x9e74d1b791e07e48, 0xc612062576589ddb, 0xf79687aed3eec551, 0x9abe14cd44753b53, 0xc16d9a0095928a27, 0xf1c90080baf72cb1, 0x971da05074da7bef, 0xbce5086492111aeb, 0xec1e4a7db69561a5, 0x9392ee8e921d5d07, 0xb877aa3236a4b449, 0xe69594bec44de15b, 0x901d7cf73ab0acd9, 0xb424dc35095cd80f, 0xe12e13424bb40e13, 0x8cbccc096f5088cc, 0xafebff0bcb24aaff, 0xdbe6fecebdedd5bf, 0x89705f4136b4a597, 0xabcc77118461cefd, 0xd6bf94d5e57a42bc, 0x8637bd05af6c69b6, 0xa7c5ac471b478423, 0xd1b71758e219652c, 0x83126e978d4fdf3b, 0xa3d70a3d70a3d70a, 0xcccccccccccccccd, 0x8000000000000000, 0xa000000000000000, 0xc800000000000000, 0xfa00000000000000, 0x9c40000000000000, 0xc350000000000000, 0xf424000000000000, 0x9896800000000000, 0xbebc200000000000, 0xee6b280000000000, 0x9502f90000000000, 0xba43b74000000000, 0xe8d4a51000000000, 0x9184e72a00000000, 0xb5e620f480000000, 0xe35fa931a0000000, 0x8e1bc9bf04000000, 0xb1a2bc2ec5000000, 0xde0b6b3a76400000, 0x8ac7230489e80000, 0xad78ebc5ac620000, 0xd8d726b7177a8000, 0x878678326eac9000, 0xa968163f0a57b400, 0xd3c21bcecceda100, 0x84595161401484a0, 0xa56fa5b99019a5c8, 0xcecb8f27f4200f3a, 0x813f3978f8940984, 0xa18f07d736b90be5, 0xc9f2c9cd04674edf, 0xfc6f7c4045812296, 0x9dc5ada82b70b59e, 0xc5371912364ce305, 0xf684df56c3e01bc7, 0x9a130b963a6c115c, 0xc097ce7bc90715b3, 0xf0bdc21abb48db20, 0x96769950b50d88f4, 0xbc143fa4e250eb31, 0xeb194f8e1ae525fd, 0x92efd1b8d0cf37be, 0xb7abc627050305ae, 0xe596b7b0c643c719, 0x8f7e32ce7bea5c70, 0xb35dbf821ae4f38c, 0xe0352f62a19e306f, 0x8c213d9da502de45, 0xaf298d050e4395d7, 0xdaf3f04651d47b4c, 0x88d8762bf324cd10, 0xab0e93b6efee0054, 0xd5d238a4abe98068, 0x85a36366eb71f041, 0xa70c3c40a64e6c52, 0xd0cf4b50cfe20766, 0x82818f1281ed44a0, 0xa321f2d7226895c8, 0xcbea6f8ceb02bb3a, 0xfee50b7025c36a08, 0x9f4f2726179a2245, 0xc722f0ef9d80aad6, 0xf8ebad2b84e0d58c, 0x9b934c3b330c8577, 0xc2781f49ffcfa6d5, 0xf316271c7fc3908b, 0x97edd871cfda3a57, 0xbde94e8e43d0c8ec, 0xed63a231d4c4fb27, 0x945e455f24fb1cf9, 0xb975d6b6ee39e437, 0xe7d34c64a9c85d44, 0x90e40fbeea1d3a4b, 0xb51d13aea4a488dd, 0xe264589a4dcdab15, 0x8d7eb76070a08aed, 0xb0de65388cc8ada8, 0xdd15fe86affad912, 0x8a2dbf142dfcc7ab, 0xacb92ed9397bf996, 0xd7e77a8f87daf7fc, 0x86f0ac99b4e8dafd, 0xa8acd7c0222311bd, 0xd2d80db02aabd62c, 0x83c7088e1aab65db, 0xa4b8cab1a1563f52, 0xcde6fd5e09abcf27, 0x80b05e5ac60b6178, 0xa0dc75f1778e39d6, 0xc913936dd571c84c, 0xfb5878494ace3a5f, 0x9d174b2dcec0e47b, 0xc45d1df942711d9a, 0xf5746577930d6501, 0x9968bf6abbe85f20, 0xbfc2ef456ae276e9, 0xefb3ab16c59b14a3, 0x95d04aee3b80ece6, 0xbb445da9ca61281f, 0xea1575143cf97227, 0x924d692ca61be758, 0xb6e0c377cfa2e12e, 0xe498f455c38b997a, 0x8edf98b59a373fec, 0xb2977ee300c50fe7, 0xdf3d5e9bc0f653e1, 0x8b865b215899f46d, 0xae67f1e9aec07188, 0xda01ee641a708dea, 0x884134fe908658b2, 0xaa51823e34a7eedf, 0xd4e5e2cdc1d1ea96, 0x850fadc09923329e, 0xa6539930bf6bff46, 0xcfe87f7cef46ff17, 0x81f14fae158c5f6e, 0xa26da3999aef774a, 0xcb090c8001ab551c, 0xfdcb4fa002162a63, 0x9e9f11c4014dda7e, 0xc646d63501a1511e, 0xf7d88bc24209a565, 0x9ae757596946075f, 0xc1a12d2fc3978937, 0xf209787bb47d6b85, 0x9745eb4d50ce6333, 0xbd176620a501fc00, 0xec5d3fa8ce427b00, 0x93ba47c980e98ce0, 0xb8a8d9bbe123f018, 0xe6d3102ad96cec1e, 0x9043ea1ac7e41393, 0xb454e4a179dd1877, 0xe16a1dc9d8545e95, 0x8ce2529e2734bb1d, 0xb01ae745b101e9e4, 0xdc21a1171d42645d, 0x899504ae72497eba, 0xabfa45da0edbde69, 0xd6f8d7509292d603, 0x865b86925b9bc5c2, 0xa7f26836f282b733, 0xd1ef0244af2364ff, 0x8335616aed761f1f, 0xa402b9c5a8d3a6e7, 0xcd036837130890a1, 0x802221226be55a65, 0xa02aa96b06deb0fe, 0xc83553c5c8965d3d, 0xfa42a8b73abbf48d, 0x9c69a97284b578d8, 0xc38413cf25e2d70e, 0xf46518c2ef5b8cd1, 0x98bf2f79d5993803, 0xbeeefb584aff8604, 0xeeaaba2e5dbf6785, 0x952ab45cfa97a0b3, 0xba756174393d88e0, 0xe912b9d1478ceb17, 0x91abb422ccb812ef, 0xb616a12b7fe617aa, 0xe39c49765fdf9d95, 0x8e41ade9fbebc27d, 0xb1d219647ae6b31c, 0xde469fbd99a05fe3, 0x8aec23d680043bee, 0xada72ccc20054aea, 0xd910f7ff28069da4, 0x87aa9aff79042287, 0xa99541bf57452b28, 0xd3fa922f2d1675f2, 0x847c9b5d7c2e09b7, 0xa59bc234db398c25, 0xcf02b2c21207ef2f, 0x8161afb94b44f57d, 0xa1ba1ba79e1632dc, 0xca28a291859bbf93, 0xfcb2cb35e702af78, 0x9defbf01b061adab, 0xc56baec21c7a1916, 0xf6c69a72a3989f5c, 0x9a3c2087a63f6399, 0xc0cb28a98fcf3c80, 0xf0fdf2d3f3c30b9f, 0x969eb7c47859e744, 0xbc4665b596706115, 0xeb57ff22fc0c795a, 0x9316ff75dd87cbd8, 0xb7dcbf5354e9bece, 0xe5d3ef282a242e82, 0x8fa475791a569d11, 0xb38d92d760ec4455, 0xe070f78d3927556b, 0x8c469ab843b89563, 0xaf58416654a6babb, 0xdb2e51bfe9d0696a, 0x88fcf317f22241e2, 0xab3c2fddeeaad25b, 0xd60b3bd56a5586f2, 0x85c7056562757457, 0xa738c6bebb12d16d, 0xd106f86e69d785c8, 0x82a45b450226b39d, 0xa34d721642b06084, 0xcc20ce9bd35c78a5, 0xff290242c83396ce, 0x9f79a169bd203e41, 0xc75809c42c684dd1, 0xf92e0c3537826146, 0x9bbcc7a142b17ccc, 0xc2abf989935ddbfe, 0xf356f7ebf83552fe, 0x98165af37b2153df, 0xbe1bf1b059e9a8d6, 0xeda2ee1c7064130c, 0x9485d4d1c63e8be8, 0xb9a74a0637ce2ee1, 0xe8111c87c5c1ba9a, 0x910ab1d4db9914a0, 0xb54d5e4a127f59c8, 0xe2a0b5dc971f303a, 0x8da471a9de737e24, 0xb10d8e1456105dad, 0xdd50f1996b947519, 0x8a5296ffe33cc930, 0xace73cbfdc0bfb7b, 0xd8210befd30efa5a, 0x8714a775e3e95c78, 0xa8d9d1535ce3b396, 0xd31045a8341ca07c, 0x83ea2b892091e44e, 0xa4e4b66b68b65d61, 0xce1de40642e3f4b9, 0x80d2ae83e9ce78f4, 0xa1075a24e4421731, 0xc94930ae1d529cfd, 0xfb9b7cd9a4a7443c, 0x9d412e0806e88aa6, 0xc491798a08a2ad4f, 0xf5b5d7ec8acb58a3, 0x9991a6f3d6bf1766, 0xbff610b0cc6edd3f, 0xeff394dcff8a948f, 0x95f83d0a1fb69cd9, 0xbb764c4ca7a44410, 0xea53df5fd18d5514, 0x92746b9be2f8552c, 0xb7118682dbb66a77, 0xe4d5e82392a40515, 0x8f05b1163ba6832d, 0xb2c71d5bca9023f8, 0xdf78e4b2bd342cf7, 0x8bab8eefb6409c1a, 0xae9672aba3d0c321, 0xda3c0f568cc4f3e9, 0x8865899617fb1871, 0xaa7eebfb9df9de8e, 0xd51ea6fa85785631, 0x8533285c936b35df, 0xa67ff273b8460357, 0xd01fef10a657842c, 0x8213f56a67f6b29c, 0xa298f2c501f45f43, 0xcb3f2f7642717713, 0xfe0efb53d30dd4d8, 0x9ec95d1463e8a507, 0xc67bb4597ce2ce49, 0xf81aa16fdc1b81db, 0x9b10a4e5e9913129, 0xc1d4ce1f63f57d73, 0xf24a01a73cf2dcd0, 0x976e41088617ca02, 0xbd49d14aa79dbc82, 0xec9c459d51852ba3, 0x93e1ab8252f33b46, 0xb8da1662e7b00a17, 0xe7109bfba19c0c9d, 0x906a617d450187e2, 0xb484f9dc9641e9db, 0xe1a63853bbd26451, 0x8d07e33455637eb3, 0xb049dc016abc5e60, 0xdc5c5301c56b75f7, 0x89b9b3e11b6329bb, 0xac2820d9623bf429, 0xd732290fbacaf134, 0x867f59a9d4bed6c0, 0xa81f301449ee8c70, 0xd226fc195c6a2f8c, 0x83585d8fd9c25db8, 0xa42e74f3d032f526, 0xcd3a1230c43fb26f, 0x80444b5e7aa7cf85, 0xa0555e361951c367, 0xc86ab5c39fa63441, 0xfa856334878fc151, 0x9c935e00d4b9d8d2, 0xc3b8358109e84f07, 0xf4a642e14c6262c9, 0x98e7e9cccfbd7dbe, 0xbf21e44003acdd2d, 0xeeea5d5004981478, 0x95527a5202df0ccb, 0xbaa718e68396cffe, 0xe950df20247c83fd, 0x91d28b7416cdd27e, 0xb6472e511c81471e, 0xe3d8f9e563a198e5, 0x8e679c2f5e44ff8f, 0xb201833b35d63f73, 0xde81e40a034bcf50, 0x8b112e86420f6192, 0xadd57a27d29339f6, 0xd94ad8b1c7380874, 0x87cec76f1c830549, 0xa9c2794ae3a3c69b, 0xd433179d9c8cb841, 0x849feec281d7f329, 0xa5c7ea73224deff3, 0xcf39e50feae16bf0, 0x81842f29f2cce376, 0xa1e53af46f801c53, 0xca5e89b18b602368, 0xfcf62c1dee382c42, 0x9e19db92b4e31ba9, 0xc5a05277621be294, 0xf70867153aa2db39, 0x9a65406d44a5c903, 0xc0fe908895cf3b44, 0xf13e34aabb430a15, 0x96c6e0eab509e64d, 0xbc789925624c5fe1, 0xeb96bf6ebadf77d9, 0x933e37a534cbaae8, 0xb80dc58e81fe95a1, 0xe61136f2227e3b0a, 0x8fcac257558ee4e6, 0xb3bd72ed2af29e20, 0xe0accfa875af45a8, 0x8c6c01c9498d8b89, 0xaf87023b9bf0ee6b, 0xdb68c2ca82ed2a06, 0x892179be91d43a44, 0xab69d82e364948d4 };
static constexpr int powers_ten_e[] = { -1203, -1200, -1196, -1193, -1190, -1186, -1183, -1180, -1176, -1173, -1170, -1166, -1163, -1160, -1156, -1153, -1150, -1146, -1143, -1140, -1136, -1133, -1130, -1127, -1123, -1120, -1117, -1113, -1110, -1107, -1103, -1100, -1097, -1093, -1090, -1087, -1083, -1080, -1077, -1073, -1070, -1067, -1063, -1060, -1057, -1053, -1050, -1047, -1043, -1040, -1037, -1034, -1030, -1027, -1024, -1020, -1017, -1014, -1010, -1007, -1004, -1000, -997, -994, -990, -987, -984, -980, -977, -974, -970, -967, -964, -960, -957, -954, -950, -947, -944, -940, -937, -934, -931, -927, -924, -921, -917, -914, -911, -907, -904, -901, -897, -894, -891, -887, -884, -881, -877, -874, -871, -867, -864, -861, -857, -854, -851, -847, -844, -841, -838, -834, -831, -828, -824, -821, -818, -814, -811, -808, -804, -801, -798, -794, -791, -788, -784, -781, -778, -774, -771, -768, -764, -761, -758, -754, -751, -748, -744, -741, -738, -735, -731, -728, -725, -721, -718, -715, -711, -708, -705, -701, -698, -695, -691, -688, -685, -681, -678, -675, -671, -668, -665, -661, -658, -655, -651, -648, -645, -642, -638, -635, -632, -628, -625, -622, -618, -615, -612, -608, -605, -602, -598, -595, -592, -588, -585, -582, -578, -575, -572, -568, -565, -562, -558, -555, -552, -549, -545, -542, -539, -535, -532, -529, -525, -522, -519, -515, -512, -509, -505, -502, -499, -495, -492, -489, -485, -482, -479, -475, -472, -469, -465, -462, -459, -455, -452, -449, -446, -442, -439, -436, -432, -429, -426, -422, -419, -416, -412, -409, -406, -402, -399, -396, -392, -389, -386, -382, -379, -376, -372, -369, -366, -362, -359, -356, -353, -349, -346, -343, -339, -336, -333, -329, -326, -323, -319, -316, -313, -309, -306, -303, -299, -296, -293, -289, -286, -283, -279, -276, -273, -269, -266, -263, -259, -256, -253, -250, -246, -243, -240, -236, -233, -230, -226, -223, -220, -216, -213, -210, -206, -203, -200, -196, -193, -190, -186, -183, -180, -176, -173, -170, -166, -163, -160, -157, -153, -150, -147, -143, -140, -137, -133, -130, -127, -123, -120, -117, -113, -110, -107, -103, -100, -97, -93, -90, -87, -83, -80, -77, -73, -70, -67, -63, -60, -57, -54, -50, -47, -44, -40, -37, -34, -30, -27, -24, -20, -17, -14, -10, -7, -4, 0, 3, 6, 10, 13, 16, 20, 23, 26, 30, 33, 36, 39, 43, 46, 49, 53, 56, 59, 63, 66, 69, 73, 76, 79, 83, 86, 89, 93, 96, 99, 103, 106, 109, 113, 116, 119, 123, 126, 129, 132, 136, 139, 142, 146, 149, 152, 156, 159, 162, 166, 169, 172, 176, 179, 182, 186, 189, 192, 196, 199, 202, 206, 209, 212, 216, 219, 222, 226, 229, 232, 235, 239, 242, 245, 249, 252, 255, 259, 262, 265, 269, 272, 275, 279, 282, 285, 289, 292, 295, 299, 302, 305, 309, 312, 315, 319, 322, 325, 328, 332, 335, 338, 342, 345, 348, 352, 355, 358, 362, 365, 368, 372, 375, 378, 382, 385, 388, 392, 395, 398, 402, 405, 408, 412, 415, 418, 422, 425, 428, 431, 435, 438, 441, 445, 448, 451, 455, 458, 461, 465, 468, 471, 475, 478, 481, 485, 488, 491, 495, 498, 501, 505, 508, 511, 515, 518, 521, 524, 528, 531, 534, 538, 541, 544, 548, 551, 554, 558, 561, 564, 568, 571, 574, 578, 581, 584, 588, 591, 594, 598, 601, 604, 608, 611, 614, 617, 621, 624, 627, 631, 634, 637, 641, 644, 647, 651, 654, 657, 661, 664, 667, 671, 674, 677, 681, 684, 687, 691, 694, 697, 701, 704, 707, 711, 714, 717, 720, 724, 727, 730, 734, 737, 740, 744, 747, 750, 754, 757, 760, 764, 767, 770, 774, 777, 780, 784, 787, 790, 794, 797, 800, 804, 807, 810, 813, 817, 820, 823, 827, 830, 833, 837, 840, 843, 847, 850, 853, 857, 860, 863, 867, 870, 873, 877, 880, 883, 887, 890, 893, 897, 900, 903, 907, 910, 913, 916, 920, 923, 926, 930, 933, 936, 940, 943, 946, 950, 953, 956, 960, 963, 966, 970, 973, 976, 980, 983, 986, 990, 993, 996, 1000, 1003, 1006, 1009, 1013, 1016, 1019, 1023, 1026, 1029, 1033, 1036, 1039, 1043, 1046, 1049, 1053, 1056, 1059, 1063, 1066, 1069, 1073, 1076 };

inline 
diy_fp_t cached_power(int k)
{
    diy_fp_t res;
    int index = 343 + k;
    res.f = powers_ten[index];
    res.e = powers_ten_e[index];
    return res;
}

// double

/*
typedef union {
    double d;
    uint64_t n;
} converter_t;

inline 
uint64_t double_to_uint64(double d) { converter_t tmp; tmp.d = d; return tmp.n; }

inline 
double uint64_to_double(uint64_t d64) { converter_t tmp; tmp.n = d64; return tmp.d; }
*/
inline 
uint64_t double_to_uint64(double d) {uint64_t d64; std::memcpy(&d64,&d,sizeof(double)); return d64; }

inline 
double uint64_to_double(uint64_t d64) {double d; std::memcpy(&d,&d64,sizeof(double)); return d; }

constexpr int dp_significand_size = 52;
constexpr int dp_exponent_bias = (0x3FF + dp_significand_size);
constexpr int dp_min_exponent = (-dp_exponent_bias);
constexpr uint64_t dp_exponent_mask = 0x7FF0000000000000;
constexpr uint64_t dp_significand_mask = 0x000FFFFFFFFFFFFF;
constexpr uint64_t dp_hidden_bit = 0x0010000000000000;

inline 
diy_fp_t normalize_diy_fp(diy_fp_t in)
{
    diy_fp_t res = in;
    /* Normalize now */
    /* the original number could have been a denormal. */
    while (!(res.f & dp_hidden_bit))
    {
        res.f <<= 1;
        res.e--;
    }
    /* do the final shifts in one go. Don't forget the hidden bit (the '-1') */
    res.f <<= (diy_significand_size - dp_significand_size - 1);
    res.e = res.e - (diy_significand_size - dp_significand_size - 1);
    return res;
}

inline 
diy_fp_t double2diy_fp(double d)
{
    uint64_t d64 = double_to_uint64(d);
    int biased_e = (d64 & dp_exponent_mask) >> dp_significand_size;
    uint64_t significand = (d64 & dp_significand_mask);
    diy_fp_t res;
    if (biased_e != 0)
    {
        res.f = significand + dp_hidden_bit;
        res.e = biased_e - dp_exponent_bias;
    } 
    else
    {
        res.f = significand;
        res.e = dp_min_exponent + 1;
    }
    return res;
}

inline 
diy_fp_t normalize_boundary(diy_fp_t in)
{
    diy_fp_t res = in;
    /* Normalize now */
    /* the original number could have been a denormal. */
    while (!(res.f & (dp_hidden_bit << 1)))
    {
        res.f <<= 1;
        res.e--;
    }
    /* do the final shifts in one go. Don't forget the hidden bit (the '-1') */
    res.f <<= (diy_significand_size - dp_significand_size - 2);
    res.e = res.e - (diy_significand_size - dp_significand_size - 2);
    return res;
}

inline 
void normalized_boundaries(double d, diy_fp_t *out_m_minus, diy_fp_t *out_m_plus)
{
    diy_fp_t v = double2diy_fp(d);
    diy_fp_t pl, mi;
    bool significand_is_zero = v.f == dp_hidden_bit;
    pl.f  = (v.f << 1) + 1; pl.e  = v.e - 1;
    pl = normalize_boundary(pl);
    if (significand_is_zero)
    {
        mi.f = (v.f << 2) - 1;
        mi.e = v.e - 2;
    } else
    {
        mi.f = (v.f << 1) - 1;
        mi.e = v.e - 1;
    }
    int x = mi.e - pl.e;
    mi.f <<= x;
    mi.e = pl.e;
    *out_m_plus = pl;
    *out_m_minus = mi;
}

inline 
double random_double()
{
    uint64_t tmp = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        tmp <<= 8;
        tmp += rand() % 256;
    }
    return uint64_to_double(tmp);
}

// grisu3

inline
bool round_weed(char *buffer, int len,
                uint64_t wp_W, uint64_t Delta,
                uint64_t rest, uint64_t ten_kappa,
                uint64_t ulp)
{
    uint64_t wp_Wup = wp_W - ulp;
    uint64_t wp_Wdown = wp_W + ulp;
    while (rest < wp_Wup &&  /// round1
           Delta - rest >= ten_kappa &&
           (rest + ten_kappa < wp_Wup || /// closer
            wp_Wup - rest >= rest + ten_kappa - wp_Wup))
    {
        buffer[len - 1]--; rest += ten_kappa;
    }
    if (rest < wp_Wdown && /// round2
        Delta - rest >= ten_kappa &&
        (rest + ten_kappa < wp_Wdown ||
         wp_Wdown - rest > rest + ten_kappa - wp_Wdown)) return 0;
    return 2 * ulp <= rest && rest <= Delta - 4 * ulp; /// weed
}

inline
bool digit_gen(diy_fp_t Wm, diy_fp_t W, diy_fp_t Wp,
               char *buffer, int *len, int *K)
{
    const uint32_t TEN2 = 100;

    uint32_t div, p1; uint64_t p2, tmp, unit = 1;
    int d, kappa;
    diy_fp_t one, wp_W, Delta;
    Delta = minus(Wp, Wm); Delta.f += 2 * unit;
    wp_W = minus(Wp, W); wp_W.f += unit;
    one.f = ((uint64_t)1) << -Wp.e; one.e = Wp.e;
    p1 = static_cast<uint32_t>((Wp.f + 1) >> -one.e);
    p2 = (Wp.f + 1) & (one.f - 1);
    *len = 0; kappa = 3; div = TEN2;
    while (kappa > 0)
    {
        d = p1 / div;
        if (d || *len) buffer[(*len)++] = (char)('0' + d);
        p1 %= div; kappa--;
        tmp = (((uint64_t)p1) << -one.e) + p2;
        if (tmp < Delta.f)
        {
            *K += kappa;
            return round_weed(buffer, *len, wp_W.f, Delta.f, tmp,
                              ((uint64_t)div) << -one.e, unit);
        }
        div /= 10;
    }
    while (1)
    {
        p2 *= 10; Delta.f *= 10; unit *= 10;
        d = static_cast<int>(p2 >> -one.e);
        if (d || *len) buffer[(*len)++] = (char)('0' + d);
        p2 &= one.f - 1; kappa--;
        if (p2 < Delta.f)
        {
            *K += kappa;
            return round_weed(buffer, *len, wp_W.f * unit, Delta.f, p2,
                              one.f, unit);
        }
    }
}

inline
bool grisu3(double v, char *buffer, int *length, int *K)
{
    diy_fp_t w_m, w_p;
    int q = 64, alpha = -59, gamma = -56;
    normalized_boundaries(v, &w_m, &w_p);
    diy_fp_t w = normalize_diy_fp(double2diy_fp(v));
    int mk = k_comp(w_p.e + q, alpha, gamma);
    diy_fp_t c_mk = cached_power(mk);
    diy_fp_t W  = multiply(w,   c_mk);
    diy_fp_t Wp = multiply(w_p, c_mk);
    diy_fp_t Wm = multiply(w_m, c_mk);
    *K = -mk;
    bool result = digit_gen(Wm, W, Wp, buffer, length, K);
    buffer[*length] = 0;
    return result;
}

} // namespace detail
} // namespace jsoncons

#endif // JSONCONS_DETAIL_GRISU3_HPP
