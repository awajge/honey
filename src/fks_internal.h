#ifndef FKS_INTERNAL_H
#define FKS_INTERNAL_H

#include <stdlib.h>

#define FKS_ID_BITS 32
#define FKS_ID_LIMIT (1ull << FKS_ID_BITS)
#define FKS_ID_MASK (FKS_ID_LIMIT - 1)

#define FKS_ESL_PAGE_BITS 12
#define FKS_ESL_PAGE_SIZE (1ull << FKS_ESL_PAGE_BITS)
#define FKS_ESL_PAGE_LIMIT (FKS_ID_LIMIT / FKS_ESL_PAGE_SIZE)
#define FKS_ESL_PAGE_MASK (FKS_ESL_PAGE_SIZE - 1)
#define FKS_ESL(wrld, id) ((wrld)->esl[(id) >> FKS_ESL_PAGE_BITS][(id) & FKS_ESL_PAGE_MASK])

#endif