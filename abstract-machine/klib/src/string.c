#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
	size_t i = 0;
	if(s==NULL) return 0;
	for (; *s; s++)
	{
		i++;
	}
	return i;
}

char *strcpy(char *dst, const char *src)
{
	assert(dst != NULL &&src!=NULL );
	const char *_src = src;
	char *_dst = dst;
	for (; *_src != '\0'; _src++)
	{
		*_dst++ = *_src;
	}
	*_dst = '\0';
	assert(strlen(dst) == strlen(src));
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
	assert(dst != NULL &&src!=NULL );
	const char *_src = src;
	char *_dst = dst;
	size_t cnt = 0;
	for (; *_src != '\0' && cnt + 1 <= n; _src++)
	{
		*_dst++ = *_src;
		cnt++;
	}
	*_dst = '\0';
	assert(strlen(dst) == cnt);
	assert(cnt <= n);
	return dst;
}

char *strcat(char *dst, const char *src)
{
	assert(dst != NULL);
	int left = strlen(dst);
	char *_dst = dst;
	const char *_src = src;
	while (*_dst != '\0')
	{
		_dst++;
	}
	assert(*_dst == '\0');
	if (src == NULL)
		return dst;
	for (; *_src != '\0'; _src++)
		*_dst++ = *_src;
	*_dst='\0';
	assert(strlen(dst) == strlen(src) + left);
	return dst;
}

int strcmp(const char *s1, const char *s2)
{
	assert(s1!=NULL && s2!=NULL );
	while(*s1!='\0' && *s2!='\0')
	{
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;
		else 
		{
			assert(*s1==*s2);
			s1++;
			s2++;
		}
	}
	if(strlen(s2)>strlen(s1)) return -1;
	else if(strlen(s2)<strlen(s1)) return 1;
	else return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	int cnt=0;
	assert(s1!=NULL && s2!=NULL );
	while(*s1!='\0' && *s2!='\0'&& cnt < n)
	{
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;
		else 
		{
			assert(*s1==*s2);
			s1++;
			s2++;
			cnt++;
		}
	}
	if(cnt >= n) return 0;
	else
	{
		if(strlen(s2)>strlen(s1)) return -1;
		else if(strlen(s2)<strlen(s1)) return 1;
		else return 0;
	}
}

void *memset(void *s, int c, size_t n)
{
	for(size_t cnt=0;cnt<n;cnt++)
	{
		((char *)s)[cnt]=c;
	}
	return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
	unsigned char *d=(unsigned char *)dst;
	const unsigned char *s=(const unsigned char *)src;
	if (dst<= src) {
		while (n--)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}
	return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
	unsigned char *d=(unsigned char *)out;
	const unsigned char *s=(const unsigned char *)in;
	while (n--)
		*d++ = *s++;
	return d;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *su1=(const unsigned char *)s1;
	const unsigned char *su2=(const unsigned char *)s2;
	size_t res = 0;
	for (; 0 < n; su1++, su2++, n--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

#endif
