#if !defined(SYSINFOS_C__)
#define SYSINFOS_C__

/**
 * Unit to read cpu informations
 *
 * tpruvot 2014
 * JayDDee 2019
 * 
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "miner.h"

#ifndef WIN32

// 1035g1: /sys/devices/platform/coretemp.0/hwmon/hwmon3/temp1_input
// 1035g1: /sys/class/hwmon/hwmon1/temp1_input wrong temp
// ryzen has no /sys/devices/platform/coretemp.0
// ryzen: /sys/class/hwmon/hwmon0
// 2400: /sys/class/hwmon/hwmon0/temp1_input incorrect temp
// 2400 has no /sys/class/hwmon/hwmon2/temp1_input
// 2400 /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input ok
// 6700 /sys/devices/platform/coretemp.0/hwmon/hwmon2/temp1_input
// 6700 /sys/class/hwmon/hwmon2/temp1_input
// /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp2_input never exists
// /sys/class/hwmon/hwmon0/temp2_input doesn't exist or shows wrong temp (sys16)
// /sys/class/hwmon/hwmon0/device/temp1_input doesn't exist


// the first 3 will find i5-2400, i7-6700k, r7-1700, i5-1035g1.
// The others are left in for legacy, some should probably be removed.
#define HWMON_PATH1 \
   "/sys/devices/platform/coretemp.0/hwmon/hwmon3/temp1_input"

#define HWMON_PATH2 \
   "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input"

#define HWMON_PATH3 \
   "/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp1_input"

#define HWMON_PATH \
 "/sys/class/hwmon/hwmon2/temp1_input"

// need this for Ryzen
#define HWMON_ALT \
 "/sys/class/hwmon/hwmon0/temp1_input"

/*
#define HWMON_ALT1 \
 "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input"
*/

// This shows wrong temp on i5-1035g1
#define HWMON_ALT2 \
 "/sys/class/hwmon/hwmon1/temp1_input"

// None of these work on any of the cpus above.
#define HWMON_ALT3 \
 "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp2_input"
#define HWMON_ALT4 \
 "/sys/class/hwmon/hwmon0/temp2_input"
#define HWMON_ALT5 \
"/sys/class/hwmon/hwmon0/device/temp1_input"

static inline float linux_cputemp(int core)
{
	float tc = 0.0;
	FILE *fd;
	uint32_t val = 0;

   fd = fopen(HWMON_PATH1, "r");

   if (!fd)
      fd = fopen(HWMON_PATH2, "r");

   if (!fd)
      fd = fopen(HWMON_PATH3, "r");

   if (!fd)
      fd = fopen(HWMON_PATH, "r");

   if (!fd)
      fd = fopen(HWMON_ALT, "r");
   
	if (!fd)
		return tc;

	if ( fscanf( fd, "%d", &val ) )
		tc = val / 1000.0;
	fclose( fd );
	return tc;
}


#define CPUFREQ_PATH0\
 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

#define CPUFREQ_PATHn \
 "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq"

static inline float linux_cpufreq(int core)
{
	FILE *fd = fopen( CPUFREQ_PATH0, "r" );
	long int freq = 0;

	if ( !fd ) return (float)freq;
	if ( !fscanf( fd, "%ld", &freq ) ) freq = 0;
   fclose( fd );
	return (float)freq;
}

static inline void linux_cpu_hilo_freq( float *lo, float *hi )
{
   long int freq = 0, hi_freq = 0, lo_freq = 0x7fffffff;

   for ( int i = 0; i < num_cpus; i++ )
   {
      char path[64];
      sprintf( path, CPUFREQ_PATHn, i );   
      FILE *fd = fopen( path, "r" );
      if ( !fd ) return;
      else if ( fscanf( fd, "%ld", &freq ) )
      {
         if ( freq > hi_freq ) hi_freq = freq;
         if ( freq < lo_freq ) lo_freq = freq;
      }
      fclose( fd );
   }
   *hi = (float)hi_freq;
   *lo = (float)lo_freq;
}


#else /* WIN32 */

static inline float win32_cputemp( int core )
{
	// todo
	return 0.0;
}


#endif /* !WIN32 */


/* exports */


static inline float cpu_temp( int core )
{
#ifdef WIN32
	return win32_cputemp( core );
#else
	return linux_cputemp( core );
#endif
}

static inline uint32_t cpu_clock( int core )
{
#ifdef WIN32
	return 0;
#else
	return linux_cpufreq( core );
#endif
}

static inline int cpu_fanpercent()
{
	return 0;
}


// CPUID

// This list is incomplete, it only contains features of interest to cpuminer.
// refer to http://en.wikipedia.org/wiki/CPUID for details.

// AVX10 compatibility notes
//
// Notation used: AVX10i.[version]_[vectorwidth]
// AVX10.1_512 is a rebranding of AVX512 and is effectively the AVX* superset
// with full 512 bit vector support.
// AVX10.2_256 is effectively AVX2 + AVX512_VL, all AVX512 instructions and
// features applied only to 256 bit and 128 bit vectors.
// Future AVX10 versions will add new instructions and features.

// Register array indexes
#define EAX_Reg  (0)
#define EBX_Reg  (1)
#define ECX_Reg  (2)
#define EDX_Reg  (3)

// CPUID function number, aka leaf (EAX)
#define VENDOR_ID            (0)
#define CPU_INFO             (1)
#define CACHE_TLB_DESCRIPTOR (2)
#define EXTENDED_FEATURES    (7)
#define AVX10_FEATURES       (0x24)
#define HIGHEST_EXT_FUNCTION (0x80000000)
#define EXTENDED_CPU_INFO    (0x80000001)
#define CPU_BRAND_1          (0x80000002)
#define CPU_BRAND_2          (0x80000003)
#define CPU_BRAND_3          (0x80000004)

// CPU_INFO: EAX=1, ECX=0
// ECX
#define SSE3_Flag                 1    
#define SSSE3_Flag               (1<< 9)
#define XOP_Flag                 (1<<11)   // obsolete
#define FMA3_Flag                (1<<12)
#define SSE41_Flag               (1<<19)
#define SSE42_Flag               (1<<20)
#define AES_NI_Flag              (1<<25)
#define XSAVE_Flag               (1<<26) 
#define OSXSAVE_Flag             (1<<27)
#define AVX_Flag                 (1<<28)
// EDX
#define MMX_Flag                 (1<<23)
#define SSE_Flag                 (1<<25)
#define SSE2_Flag                (1<<26) 

// EXTENDED_FEATURES subleaf 0: EAX=7, ECX=0
// EBX
#define AVX2_Flag                (1<< 5)
#define AVX512_F_Flag            (1<<16)
#define AVX512_DQ_Flag           (1<<17)
#define AVX512_IFMA_Flag         (1<<21)
#define AVX512_PF_Flag           (1<<26)
#define AVX512_ER_Flag           (1<<27)
#define AVX512_CD_Flag           (1<<28)
#define SHA_Flag                 (1<<29)
#define AVX512_BW_Flag           (1<<30)
#define AVX512_VL_Flag           (1<<31)
// ECX
#define AVX512_VBMI_Flag         (1<< 1) 
#define AVX512_VBMI2_Flag        (1<< 6)
#define VAES_Flag                (1<< 9)
#define AVX512_VNNI_Flag         (1<<11)
#define AVX512_BITALG_Flag       (1<<12)
#define AVX512_VPOPCNTDQ_Flag    (1<<14)
// EDX
#define AVX512_4VNNIW_Flag       (1<< 2)
#define AVX512_4FMAPS_Flag       (1<< 3)
#define AVX512_VP2INTERSECT_Flag (1<< 8)
#define AMX_BF16_Flag            (1<<22)
#define AVX512_FP16_Flag         (1<<23)
#define AMX_TILE_Flag            (1<<24)
#define AMX_INT8_Flag            (1<<25)

// EXTENDED_FEATURES subleaf 1: EAX=7, ECX=1
// EAX
#define SHA512_Flag               1
#define SM3_Flag                 (1<< 1)
#define SM4_Flag                 (1<< 2)
#define AVX_VNNI_Flag            (1<< 4)
#define AVX512_BF16_Flag         (1<< 5)
#define AMX_FP16_Flag            (1<<21)
#define AVX_IFMA_Flag            (1<<23)
// EDX
#define AVX_VNNI_INT8_Flag       (1<< 4)
#define AVX_NE_CONVERT_Flag      (1<< 5)
#define AMX_COMPLEX_Flag         (1<< 8)
#define AVX_VNNI_INT16_Flag      (1<<10)
#define AVX10_Flag               (1<<19)
#define APX_F_Flag               (1<<21)

// AVX10_FEATURES: EAX=0x24, ECX=0
// EBX
#define AVX10_VERSION_mask        0xff      // bits [7:0]
#define AVX10_128_Flag           (1<<16)
#define AVX10_256_Flag           (1<<17)   
#define AVX10_512_Flag           (1<<18)   

// Use this to detect presence of feature
#define AVX_mask     (AVX_Flag|XSAVE_Flag|OSXSAVE_Flag)
#define FMA3_mask    (FMA3_Flag|AVX_mask)
#define AVX512_mask  (AVX512_VL_Flag|AVX512_BW_Flag|AVX512_DQ_Flag|AVX512_F_Flag)


#ifndef __arm__
static inline void cpuid( unsigned int leaf, unsigned int subleaf,
                          unsigned int output[4] )
{
#if defined (_MSC_VER) || defined (__INTEL_COMPILER)
   // Microsoft or Intel compiler, intrin.h included
   __cpuidex(output, leaf, subleaf );
#elif defined(__GNUC__) || defined(__clang__)
   // use inline assembly, Gnu/AT&T syntax
   unsigned int a, b, c, d;
   asm volatile( "cpuid"
               : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
               : "a"(leaf), "c"(subleaf) );
   output[ EAX_Reg ] = a;
   output[ EBX_Reg ] = b;
   output[ ECX_Reg ] = c;
   output[ EDX_Reg ] = d;
#else
   // unknown platform. try inline assembly with masm/intel syntax
   __asm {
      mov eax, leaf
      mov ecx, subleaf
      cpuid;
      mov esi, output
      mov[esi], eax
      mov[esi + 4], ebx
      mov[esi + 8], ecx
      mov[esi + 12], edx
   }
#endif
}
#else /* !__arm__ */
#define cpuid(leaf, subleaf, out) out[0] = 0;
#endif

static inline void cpu_getname(char *outbuf, size_t maxsz)
{
   memset(outbuf, 0, maxsz);
#ifdef WIN32
   char brand[256] = { 0 };
   int output[4] = { 0 }, ext;
   cpuid( 0x80000000, 0, output );
   ext = output[0];
   if (ext >= 0x80000004)
   {
      for (int i = 2; i <= (ext & 0xF); i++)
      {
         cpuid( 0x80000000+i, 0, output);
         memcpy(&brand[(i-2) * 4*sizeof(int)], output, 4*sizeof(int));
      }
      snprintf(outbuf, maxsz, "%s", brand);
   }
   else
   {
      // Fallback, for the i7-5775C will output
      // Intel64 Family 6 Model 71 Stepping 1, GenuineIntel
      snprintf(outbuf, maxsz, "%s", getenv("PROCESSOR_IDENTIFIER"));
   }
#else
   // Intel(R) Xeon(R) CPU E3-1245 V2 @ 3.40GHz
   FILE *fd = fopen("/proc/cpuinfo", "rb");
   char *buf = NULL, *p, *eol;
   size_t size = 0;
   if (!fd) return;
   while(getdelim(&buf, &size, 0, fd) != -1)
   {
      if (buf && (p = strstr(buf, "model name\t")) && strstr(p, ":"))
      {
          p = strstr(p, ":");
          if (p)
          {
              p += 2;
	      eol = strstr(p, "\n"); if (eol) *eol = '\0';
	      snprintf(outbuf, maxsz, "%s", p);
          }
          break;
        }
   }
   free(buf);
   fclose(fd);
#endif
}

static inline void cpu_getmodelid(char *outbuf, size_t maxsz)
{
   memset(outbuf, 0, maxsz);
#ifdef WIN32
   // For the i7-5775C will output 6:4701:8
   snprintf(outbuf, maxsz, "%s:%s:%s", getenv("PROCESSOR_LEVEL"), // hexa ?
   getenv("PROCESSOR_REVISION"), getenv("NUMBER_OF_PROCESSORS"));
#else
   FILE *fd = fopen("/proc/cpuinfo", "rb");
   char *buf = NULL, *p;
   int cpufam = 0, model = 0, stepping = 0;
   size_t size = 0;
   if (!fd) return;

   while(getdelim(&buf, &size, 0, fd) != -1)
   {
      if (buf && (p = strstr(buf, "cpu family\t")) && strstr(p, ":"))
      {
         p = strstr(p, ":");
	 if (p)
         {
	    p += 2;
	    cpufam = atoi(p);
	 }
      }
      if (buf && (p = strstr(buf, "model\t")) && strstr(p, ":"))
      {
         p = strstr(p, ":");
	 if (p)
         {
            p += 2;
            model = atoi(p);
         }
      }
      if (buf && (p = strstr(buf, "stepping\t")) && strstr(p, ":"))
      {
         p = strstr(p, ":");
         if (p)
         {
            p += 2;
            stepping = atoi(p);
         }
      }
      if (cpufam && model && stepping)
      {
         snprintf( outbuf, maxsz, "%x:%02x%02x:%d", cpufam, model, stepping,
                   num_cpus);
         outbuf[maxsz-1] = '\0';
         break;
      }
   }
   free(buf);
   fclose(fd);
#endif
}
 
// Typical display format: AVX10.[version]_[vectorlength], if vector length is
// omitted 256 is the default.
//    Ex: AVX10.1_512
// Flags:
// AVX10  128  256  512
//   0     0    0    0    = AVX10 not supported
//   1     1    1    0    = AVX10 256 bit max  (version 2)
//   1     1    1    1    = AVX10 512 bit max  (version 1 granite rapids)
// Other combinations are not defined.

// Test AVX10_flag before AVX10_FEATURES flags.
static inline bool has_avx10()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 1, cpu_info );
    return cpu_info[ EDX_Reg ] & AVX10_Flag;
#endif
}

static inline unsigned int avx10_version()
{
#ifdef __arm__
    return 0;
#else
    if ( has_avx10() )
    {
       unsigned int cpu_info[4] = { 0 };
       cpuid( AVX10_FEATURES, 0, cpu_info );
       return cpu_info[ EBX_Reg ] & AVX10_VERSION_mask;
    }
    return 0;
#endif
}

static inline bool has_avx10_512()
{
#ifdef __arm__
    return false;
#else
    if ( has_avx10() )
    {
       unsigned int cpu_info[4] = { 0 };
       cpuid( AVX10_FEATURES, 0, cpu_info );
       return cpu_info[ EBX_Reg ] & AVX10_512_Flag;
    }
    return false;
#endif
}

static inline bool has_avx10_256()
{
#ifdef __arm__
    return false;
#else
    if ( has_avx10() )
    {
       unsigned int cpu_info[4] = { 0 };
       cpuid( AVX10_FEATURES, 0, cpu_info );
       return cpu_info[ EBX_Reg ] & AVX10_256_Flag;
    }
    return false;
#endif
}

// Maximum vector length
static inline unsigned int avx10_vector_length()
{
#ifdef __arm__
    return 0;
#else
    if ( has_avx10() )
    {
       unsigned int cpu_info[4] = { 0 };
       cpuid( AVX10_FEATURES, 0, cpu_info );
       return cpu_info[ EBX_Reg ] & AVX10_512_Flag ? 512
          : ( cpu_info[ EBX_Reg ] & AVX10_256_Flag ? 256 : 0 );
    }
    return 0;
#endif
}    

static inline bool has_sha()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & SHA_Flag;
#endif
}

static inline bool has_sse2()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( CPU_INFO, 0, cpu_info );
    return cpu_info[ EDX_Reg ] & SSE2_Flag;
#endif
}

// nehalem and above, no AVX on nehalem
static inline bool has_aes_ni()
{
#ifdef __arm__
	return false;
#else
	unsigned int cpu_info[4] = { 0 };
        cpuid( CPU_INFO, 0, cpu_info );
	return cpu_info[ ECX_Reg ] & AES_NI_Flag;
#endif
}

// westmere and above
static inline bool has_avx()
{
#ifdef __arm__
        return false;
#else
        unsigned int cpu_info[4] = { 0 };
        cpuid( CPU_INFO, 0, cpu_info );
        return ( ( cpu_info[ ECX_Reg ] & AVX_mask ) == AVX_mask );
#endif
}

// haswell and above
static inline bool has_avx2()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & AVX2_Flag;
#endif
}

static inline bool has_avx512f()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & AVX512_F_Flag;
#endif
}

static inline bool has_avx512dq()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & AVX512_DQ_Flag;
#endif
}

static inline bool has_avx512bw()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & AVX512_BW_Flag;
#endif
}

static inline bool has_avx512vl()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ EBX_Reg ] & AVX512_VL_Flag;
#endif
}

// Minimum to be useful
static inline bool has_avx512()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return ( ( cpu_info[ EBX_Reg ] & AVX512_mask ) == AVX512_mask );
#endif
}

static inline bool has_vaes()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ ECX_Reg ] & VAES_Flag;
#endif
}

static inline bool has_vbmi()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ ECX_Reg ] & AVX512_VBMI_Flag;
#endif
}

static inline bool has_vbmi2()
{
#ifdef __arm__
    return false;
#else
    unsigned int cpu_info[4] = { 0 };
    cpuid( EXTENDED_FEATURES, 0, cpu_info );
    return cpu_info[ ECX_Reg ] & AVX512_VBMI2_Flag;
#endif
}

// AMD only
static inline bool has_xop()
{
#ifdef __arm__
        return false;
#else
        unsigned int cpu_info[4] = { 0 };
        cpuid( EXTENDED_CPU_INFO, 0, cpu_info );
        return cpu_info[ ECX_Reg ] & XOP_Flag;
#endif
}

static inline bool has_fma3()
{
#ifdef __arm__
        return false;
#else
        unsigned int cpu_info[4] = { 0 };
        cpuid( CPU_INFO, 0, cpu_info );
        return ( ( cpu_info[ ECX_Reg ] & FMA3_mask ) == FMA3_mask );
#endif
}

static inline bool has_sse42()
{
#ifdef __arm__
        return false;
#else
        unsigned int cpu_info[4] = { 0 };
        cpuid( CPU_INFO, 0, cpu_info );
        return cpu_info[ ECX_Reg ] & SSE42_Flag;
#endif
}

static inline bool has_sse()
{
#ifdef __arm__
        return false;
#else
        unsigned int cpu_info[4] = { 0 };
        cpuid( CPU_INFO, 0, cpu_info );
        return cpu_info[ EDX_Reg ] & SSE_Flag;
#endif
}

static inline uint32_t cpuid_get_highest_function_number()
{
  unsigned int cpu_info[4] = {0};
  cpuid( VENDOR_ID, 0, cpu_info);
  return cpu_info[ EAX_Reg ];
}

static inline void cpuid_get_highest_function( char* s )
{
  uint32_t fn = cpuid_get_highest_function_number();
  switch (fn)
  {
    case 0x16:
      strcpy( s, "Skylake" );
      break;
    case 0xd:
      strcpy( s, "IvyBridge" );
      break;
    case 0xb:
      strcpy( s, "Corei7" );
      break;
    case 0xa:
      strcpy( s, "Core2" );
      break;
    default:
      sprintf( s, "undefined %x", fn );
  }
}

static inline void cpu_bestfeature(char *outbuf, size_t maxsz)
{
#ifdef __arm__
	sprintf(outbuf, "ARM");
#else
	int cpu_info[4] = { 0 };
	int cpu_info_adv[4] = { 0 };
	cpuid( CPU_INFO, 0, cpu_info );
	cpuid( EXTENDED_FEATURES, 0, cpu_info_adv );

        if ( has_avx() && has_avx2() )
              sprintf(outbuf, "AVX2");
        else if ( has_avx() )
              sprintf(outbuf, "AVX");
        else if ( has_fma3() )
              sprintf(outbuf, "FMA3");
        else if ( has_xop() )
              sprintf(outbuf, "XOP");
        else if ( has_sse42() )
              sprintf(outbuf, "SSE42");
        else if ( has_sse2() )
              sprintf(outbuf, "SSE2");
        else if ( has_sse() )
              sprintf(outbuf, "SSE");
        else
              *outbuf = '\0';
           
#endif
}

static inline void cpu_brand_string( char* s )
{
#ifdef __arm__
        sprintf( s, "ARM" );
#else
    int cpu_info[4] = { 0 };
    cpuid( VENDOR_ID, 0, cpu_info );
    if ( cpu_info[ EAX_Reg ] >= 4 )
    {
        cpuid( CPU_BRAND_1, 0, cpu_info );
        memcpy( s, cpu_info, sizeof(cpu_info) );
        cpuid( CPU_BRAND_2, 0, cpu_info );
        memcpy( s + 16, cpu_info, sizeof(cpu_info) );
        cpuid( CPU_BRAND_3, 0, cpu_info );
        memcpy( s + 32, cpu_info, sizeof(cpu_info) );
    }
#endif
}    

#endif  // SYSINFOS_C__

