#ifndef H_FOWLTALK_DEBUG
#define H_FOWLTALK_DEBUG

#if FT_DEBUG
# include <stdio.h>
# include <string.h>
# define ft_debug(x, ...)      do{fprintf(stderr, "%s:%s(%u): " x "\n", strstr(__FILE__, "src/"), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)
#else
# define ft_debug(x, ...)      /* x */
#endif

#endif