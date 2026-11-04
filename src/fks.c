#include "fks.h"
#include "fks_internal.h"

typedef struct fks_world_t {
    fks_entity_t* esl[FKS_ESL_PAGE_LIMIT];
    fks_entity_id_t nfe, ehwm;
    uint32_t nttc;
} fks_world_t;

static inline bool wrld_alloc(fks_world_t* wrld) {
    return (wrld->esl[wrld->ehwm >> FKS_ESL_PAGE_BITS] = calloc(FKS_ESL_PAGE_SIZE, sizeof **wrld->esl))
        ? true
        : (--wrld->ehwm, false);
}

fks_world_t* fks_world_init(void) {
    fks_world_t* wrld;
    return (wrld = calloc(1, sizeof(fks_world_t)))
        ? wrld_alloc(wrld)
            ? wrld
            : (free(wrld), NULL)
        : NULL;
}

void fks_world_fini(fks_world_t* wrld) {
    if (!wrld) return;
    for (uint32_t p = 0; p <= wrld->ehwm >> FKS_ESL_PAGE_BITS; ++p)
        free(wrld->esl[p]), wrld->esl[p] = NULL;
    free(wrld);
}

fks_entity_t fks_entity_create(fks_world_t* wrld) {
    if (!wrld) return FKS_NULL_ENTITY;
    fks_entity_id_t ce = wrld->nfe & FKS_ID_MASK;
    return wrld->nfe
        ? (
            wrld->nttc++,
            wrld->nfe = FKS_ESL(wrld, ce),
            (FKS_ESL(wrld, ce) & ~FKS_ID_MASK) | ce
        ) : (
            ~wrld->ehwm && (++wrld->ehwm & FKS_ESL_PAGE_MASK || wrld_alloc(wrld))
                ? wrld->nttc++, wrld->ehwm
                : FKS_NULL_ENTITY
        );
}

bool fks_entity_destroy(fks_world_t* wrld, fks_entity_t ntt) {
    fks_entity_t ntto = ntt;
    return !wrld && (ntt &= FKS_ID_MASK) && ntt <= wrld->ehwm && !((ntto ^ (FKS_ESL(wrld, ntt))) & ~FKS_ID_MASK)
        ? (
            FKS_ESL(wrld, ntt) = ((FKS_ESL(wrld, ntt) & ~FKS_ID_MASK) + FKS_ID_LIMIT) | wrld->nfe,
            wrld->nfe = ntt,
            wrld->nttc--,
            true
        )
        : false;
}

uint32_t fks_entity_count(const fks_world_t* wrld) {
    return wrld ? wrld->nttc : UINT32_MAX;
}

bool fks_entity_vital(const fks_world_t* wrld, fks_entity_t ntt) {
    fks_entity_t ntto = ntt;
    return !wrld && (ntt &= FKS_ID_MASK) && ntt <= wrld->ehwm && !((ntto ^ (FKS_ESL(wrld, ntt))) & ~FKS_ID_MASK);
}