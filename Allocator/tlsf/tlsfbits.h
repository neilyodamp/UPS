#ifndef INCLUDED_tlsfbits
#define INCLUDED_tlsfbits

#if defined(__cplusplus)
#define tlsf_decl inline
#else
#define tlsf_decl static
#endif

/*

特定架构的位操作程序.

TLSF通过将空闲块的搜索限制到足以满足请求的保证大小的空闲列表
以及使用位掩码和特定于体系结构的位操作程序的高效空闲列表查询来实现对于malloc和free操作的O（1）成本。

大多数现代处理器提供指令来计算一个字中的前导零，找到最低和最高的设置位等。这些具体实现将在可用时被使用，回到相当有效的通用实现。

大多数现代处理器提供指令计算一个word(4字节,机器不同字节数不同)前面有几个零,找到最低位和最高位.等等
在特定的情况下处理器下有特殊的处理，如果没有特殊处理就采用通用的处理方法

TLSF规范依赖于 ffs/fls 返回值 0..31

检测是否建立一个32位或64位（LP / LLP）架构。 编译时没有可靠的便携式方法。

*/

// native client 使用 ILP32(数据模型,可google查询什么意思), 即使是由x86_64指令架构建立, 
// 所以要确定 指针的大小为32位(bits)

#if !defined(__native_client__)
#if defined (__alpha__) || defined (__ia64__) || defined (__x86_64__) \
	|| defined (_WIN64) || defined (__LP64__) || defined (__LLP64__)
#define TLSF_64BIT
#endif
#endif
/*
gcc 3.4和以上版本有内置支持特定架构
一些编译器会冒充成gcc, patchlevel测试会过滤掉他们
*/
#if defined (__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) \
	&& defined (__GNUC_PATCHLEVEL__)

tlsf_decl int tlsf_ffs(unsigned int word)
{
    // __builtin_ffs 是gcc 内置函数,
    //__builtin_ffs(x)作用是: 返回 x中最后一个为1的位是从后向前的第几位,如 __builtin_ffs(0x789) = 1,__builtin_ffs(0x78c) = 3.
    //所以 __builtin_ffs(x) - 1 的作用是 返回x中最后一个为1的位的位置.
	return __builtin_ffs(word) - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
    //__builtin_clz(x) 作用是: 返回x前导0的个数 ,x =0 结果未定义.
    //所以这里将对 word 为 0 单独讨论.
	const int bit = word ? 32 - __builtin_clz(word) : 0;
    return bit - 1;
}

#elif defined (_MSC_VER) && defined (_M_IX86) && (_MSC_VER >= 1400)
/* Microsoft Visual C++ 2005 支持 x86 架构. */

//这里涉及一点C++ ，但阅读起来应该不难
#include <intrin.h>

#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)

tlsf_decl int tlsf_fls(unsigned int word)
{
	unsigned long index;

    // _BitScanReverse 将 mask 数据 从最高有效位(MSB)到
	//参数1:
    //参数2:
    return _BitScanReverse(&index, word) ? index : -1;
}

tlsf_decl int tlsf_ffs(unsigned int word)
{
	unsigned long index;
	return _BitScanForward(&index, word) ? index : -1;
}

#elif defined (_MSC_VER) && defined (_M_PPC)
/* Microsoft Visual C++ support on PowerPC architectures. */

#include <ppcintrinsics.h>

tlsf_decl int tlsf_fls(unsigned int word)
{
	const int bit = 32 - _CountLeadingZeros(word);
	return bit - 1;
}

tlsf_decl int tlsf_ffs(unsigned int word)
{
	const unsigned int reverse = word & (~word + 1);
	const int bit = 32 - _CountLeadingZeros(reverse);
	return bit - 1;
}

#elif defined (__ARMCC_VERSION)
/* RealView Compilation Tools for ARM */

tlsf_decl int tlsf_ffs(unsigned int word)
{
	const unsigned int reverse = word & (~word + 1);
	const int bit = 32 - __clz(reverse);
	return bit - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
	const int bit = word ? 32 - __clz(word) : 0;
	return bit - 1;
}

#elif defined (__ghs__)
/* Green Hills support for PowerPC */

#include <ppc_ghs.h>

tlsf_decl int tlsf_ffs(unsigned int word)
{
	const unsigned int reverse = word & (~word + 1);
	const int bit = 32 - __CLZ32(reverse);
	return bit - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
	const int bit = word ? 32 - __CLZ32(word) : 0;
	return bit - 1;
}

#else
/* Fall back to generic implementation. */

tlsf_decl int tlsf_fls_generic(unsigned int word)
{
	int bit = 32;

	if (!word) bit -= 1;
	if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
	if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
	if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
	if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
	if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }

	return bit;
}

/* Implement ffs in terms of fls. */
tlsf_decl int tlsf_ffs(unsigned int word)
{
	return tlsf_fls_generic(word & (~word + 1)) - 1;
}

tlsf_decl int tlsf_fls(unsigned int word)
{
	return tlsf_fls_generic(word) - 1;
}

#endif

/* Possibly 64-bit version of tlsf_fls. */
#if defined (TLSF_64BIT)
tlsf_decl int tlsf_fls_sizet(size_t size)
{
	int high = (int)(size >> 32);
	int bits = 0;
	if (high)
	{
		bits = 32 + tlsf_fls(high);
	}
	else
	{
		bits = tlsf_fls((int)size & 0xffffffff);

	}
	return bits;
}
#else
#define tlsf_fls_sizet tlsf_fls
#endif

#undef tlsf_decl

#endif
