/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Compiling for Android! */
/* #undef ANDROID */

/* build number based on the version control system */
#define BUILD_NUMBER "6ba8f6b"

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* "daemon library has daemon_reset_sigs() and daemon_unblock_sigs()
   functions" */
#define HAVE_DAEMON_RESET_SIGS 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* "curl library is available" */
#define HAVE_LIBCURL 1

/* "daemon library is available" */
#define HAVE_LIBDAEMON 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `rt' library (-lrt). */
#define HAVE_LIBRT 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* "enable lowpan support" */
#define HAVE_LOWPAN_SUPPORT 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* "sqlite library is available" */
#define HAVE_SQLITE 1

/* Define to 1 if stdbool.h conforms to C99. */
/* #undef HAVE_STDBOOL_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/inotify.h> header file. */
#define HAVE_SYS_INOTIFY_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* "Email Convergence Layer is available" */
/* #undef HAVE_VMIME */

/* Define to 1 if you have the <vmime/utility/smartPtrInt.hpp> header file. */
/* #undef HAVE_VMIME_UTILITY_SMARTPTRINT_HPP */

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "ibrdtnd"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "morgenro@ibr.cs.tu-bs.de"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ibrdtnd"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "ibrdtnd 0.12.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "ibrdtnd"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.12.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "0.12.0"

/* Compiling for Win32! */
/* #undef WIN32 */

/* Minimum platform is Windows 7 */
/* #undef WINVER */

/* "bundle security support enabled" */
#define WITH_BUNDLE_SECURITY 1

/* "bundle compression support enabled" */
#define WITH_COMPRESSION 1

/* "dht nameservice support enabled" */
#define WITH_DHT_NAMESERVICE 1

/* "tls support enabled" */
#define WITH_TLS 1

/* "wifi-p2p support enabled" */
#define WITH_WIFIP2P 1

/* Minimum platform is Windows 7 */
/* #undef _WIN32_WINNT */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
