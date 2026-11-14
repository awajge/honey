#include "fks.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* =============== Entity ID =============== */
#define FKS_ID_BITS     (sizeof(fks_entity_id_t) * 8)
#define FKS_ID_LIMIT    (1ull << FKS_ID_BITS)
#define FKS_ID_MASK     (FKS_ID_LIMIT - 1)
#define FKS_ID(ntt)     ((fks_entity_id_t) ((ntt) & FKS_ID_MASK))

typedef uint32_t        fks_entity_id_t;

/* =============== Entity State List (ESL) | 2nd Level Paged List =============== */
#define FKS_ESL0_PAGE_BITS  12
#define FKS_ESL1_PAGE_BITS  20

#define FKS_ESL0_PAGE_SIZE  (1ull << FKS_ESL0_PAGE_BITS)
#define FKS_ESL1_PAGE_SIZE  (1ull << FKS_ESL1_PAGE_BITS)

#define FKS_ESL0_IDX(ntt)   ((FKS_ID(ntt))                         & (FKS_ESL0_PAGE_SIZE - 1))
#define FKS_ESL1_IDX(ntt)   (((FKS_ID(ntt)) >> FKS_ESL0_PAGE_BITS) & (FKS_ESL1_PAGE_SIZE - 1))
#define FKS_ESL(wrld, ntt)  ((*(wrld)->esl[FKS_ESL1_IDX(ntt)])[FKS_ESL0_IDX(ntt)])

#define FKS_ESL0_CALLOC(esl0ptr)    ((esl0ptr) = calloc(FKS_ESL0_PAGE_SIZE, sizeof **(esl0ptr)))

typedef fks_entity_t        fks_esl0[FKS_ESL0_PAGE_SIZE];
typedef fks_esl0*           fks_esl1[FKS_ESL1_PAGE_SIZE];

/* =============== Component Sparse Set (CSS) | 3rd Level Paged List  =============== */
#define FKS_CSS0_PAGE_BITS  10
#define FKS_CSS1_PAGE_BITS  11
#define FKS_CSS2_PAGE_BITS  11

#define FKS_CSS0_PAGE_SIZE  (1ull << FKS_CSS0_PAGE_BITS)
#define FKS_CSS1_PAGE_SIZE  (1ull << FKS_CSS1_PAGE_BITS)
#define FKS_CSS2_PAGE_SIZE  (1ull << FKS_CSS2_PAGE_BITS)

#define FKS_CSS0_IDX(ntt)   ((FKS_ID(ntt))                                                & (FKS_CSS0_PAGE_SIZE - 1))
#define FKS_CSS1_IDX(ntt)   (((FKS_ID(ntt)) >> FKS_CSS0_PAGE_BITS)                        & (FKS_CSS1_PAGE_SIZE - 1))
#define FKS_CSS2_IDX(ntt)   (((FKS_ID(ntt)) >> (FKS_CSS0_PAGE_BITS + FKS_CSS1_PAGE_BITS)) & (FKS_CSS2_PAGE_SIZE - 1))
#define FKS_CSS(cmpd, ntt)  ((*(*(cmpd)->css[FKS_CSS2_IDX(ntt)])[FKS_CSS1_IDX(ntt)])[FKS_CSS0_IDX(ntt)])

typedef fks_entity_id_t     fks_css0[FKS_CSS0_PAGE_SIZE];
typedef fks_css0*           fks_css1[FKS_CSS1_PAGE_SIZE];
typedef fks_css1*           fks_css2[FKS_CSS2_PAGE_SIZE];

/* =============== Component Data =============== */
#define FKS_COMPONENT_DENSE_ICAP    64

#define FKS_COMPONENT_LIMIT         (1ull << (sizeof(fks_component_t) * 8))
#define FKS_COMPONENT_MAX           (FKS_COMPONENT_LIMIT - 1)

/* =============== Error Logging =============== */
#define FKS_LOG_MAX_BUF_SIZE            128

#define FKS_LOG(loglv, errt, ...)       fks_error(#loglv, errt, __func__, __LINE__, __VA_ARGS__)
#define FKS_LOG_GENERAL(loglv, msg)     FKS_LOG(loglv, "General", (msg))
#define FKS_LOG_BAD_INPUT(rec, type)    FKS_LOG(FKS_WARNING, "Input", "Bad input: %s for %s", (rec), (type))
#define FKS_LOG_NULL_INPUT(type)        FKS_LOG_BAD_INPUT("NULL", (type))
#define FKS_LOG_ALLOC(type)             FKS_LOG(FKS_ERROR, "Alloc", "Failed to allocate resource: %s", (type))

typedef enum {
    FKS_WARNING,
    FKS_ERROR
} fks_log_level;

/* =============== General ===============*/
typedef unsigned char fks_uc;

/* =============== Structs =============== */
typedef struct fks_component_data_t {
    fks_css2 css;        /**< Component Sparse Set     */
    fks_entity_t* rds;   /**< Reverse Dense Set        */
    void* cds;           /**< Component Dense Set      */

    fks_entity_id_t dsc; /**< Dense Set Capacity       */
    fks_entity_id_t nfs; /**< Next Free Slot           */
    uint32_t scs;        /**< Single Component Size    */
    bool ccs;            /**< Component Creation State */
} fks_component_data_t;

struct fks_world_t {
    fks_esl1 esl;                                    /**< Entity State List       */
    fks_component_data_t* cdr[FKS_COMPONENT_LIMIT];  /**< Component Data Registry */

    uint32_t nttc;                                   /**< Entity Count             */
    fks_entity_id_t nfe;                             /**< Next Free Entity         */
    fks_entity_id_t ehwm;                            /**< Entity High Watermark    */
    fks_component_t chwm;                            /**< Component High Watermark */
};

/* =============== Internal Funcs =============== */
static inline void fks_error(const char* lv, const char* errt, const char* func, int line, const char* format, ...) {
    char buf[FKS_LOG_MAX_BUF_SIZE];
    va_list argv; va_start(argv, format);
    vsnprintf(buf, sizeof(buf), format, argv);
    va_end(argv);

    fprintf(stderr, "%s %s @ [%s:%d] - %s\n", lv, errt, func, line, buf);
}

static inline bool wrld_valid(const fks_world_t* wrld) {
    if (!wrld) return FKS_LOG_NULL_INPUT("fks_world_t*"), false;

    return true;
}

static inline bool wrld_ntt_valid(const fks_world_t* wrld, fks_entity_t ntt) {
    fks_entity_t ntto = ntt;
    if (!wrld_valid(wrld))                              return false;
    if (!(ntt &= FKS_ID_MASK))                          return FKS_LOG_NULL_INPUT("fks_entity_t"), false;
    if (ntt > wrld->ehwm)                               return FKS_LOG_BAD_INPUT("invalid entity", "fks_entity_t"), false;
    if ((ntto ^ (FKS_ESL(wrld, ntt))) & ~FKS_ID_MASK)   return FKS_LOG_BAD_INPUT("stale entity", "fks_entity_t"), false;

    return true;
}

static inline bool wrld_cmp_valid(const fks_world_t* wrld, fks_component_t cmp) {
    if (!wrld_valid(wrld))  return false;
    if (!cmp)               return FKS_LOG_NULL_INPUT("fks_component_t"), false;
    if (cmp > wrld->chwm)   return FKS_LOG_BAD_INPUT("invalid component", "fks_component_t"), false;

    return true;
}

static inline bool wrld_ntt_cmp_valid(const fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp) {
    return wrld_ntt_valid(wrld, ntt) && wrld_cmp_valid(wrld, cmp);
}

/* =============== Implementation =============== */
fks_world_t* fks_world_init(void) {
    fks_world_t* wrld = calloc(1, sizeof(fks_world_t));
    return wrld ? wrld : (FKS_LOG_ALLOC("fks_world_t*"), NULL);
}

void fks_world_fini(fks_world_t* wrld) {
    if (!wrld) return;

    for (size_t i = 0; i <= (wrld->ehwm >> FKS_ESL0_PAGE_BITS); ++i) free(wrld->esl[i]);
    
    fks_component_data_t* cmpd; fks_css1* tssl1;
    for (size_t cmp = 1; cmp <= wrld->chwm; ++cmp) {
        cmpd = wrld->cdr[cmp];
        for (size_t i = 0; i < FKS_CSS2_PAGE_SIZE; ++i)
            if ((tssl1 = cmpd->css[i]))
                for (size_t j = 0; j < FKS_CSS1_PAGE_SIZE; ++j) free((*tssl1)[j]);
        free(cmpd->rds), free(cmpd->cds);
    }

    free(wrld);
}

uint32_t fks_world_entity_count(const fks_world_t* wrld) {
    return wrld_valid(wrld) ? wrld->nttc : UINT32_MAX;
}

fks_entity_t fks_entity_create(fks_world_t* wrld) {
    if (!wrld_valid(wrld)) return FKS_NULL_ENTITY;

    if (wrld->nfe) {
        fks_entity_id_t _nfe = wrld->nfe;
        wrld->nttc++;
        wrld->nfe = FKS_ID(FKS_ESL(wrld, _nfe));
        
        return (FKS_ESL(wrld, _nfe) & ~FKS_ID_MASK) | _nfe;
    } else {
        if (wrld->ehwm == FKS_ID_MASK) return FKS_LOG_GENERAL(FKS_ERROR, "Entity limit reached"), FKS_NULL_ENTITY;
        if (!(wrld->ehwm % (FKS_ESL0_PAGE_SIZE - 1) || FKS_ESL0_CALLOC(wrld->esl[FKS_ESL1_IDX(wrld->ehwm + 1)])))
            return FKS_LOG_ALLOC("fks_esl0*"), FKS_NULL_ENTITY;
        
        return wrld->nttc++, ++wrld->ehwm;
    }
}

bool fks_entity_destroy(fks_world_t* wrld, fks_entity_t ntt) {
    if (!wrld_ntt_valid(wrld, ntt)) return false;

    for (fks_component_t cmp = 1; cmp <= wrld->chwm; ++cmp) fks_entity_remove_component(wrld, ntt, cmp);
    FKS_ESL(wrld, ntt) = ((FKS_ESL(wrld, ntt) & ~FKS_ID_MASK) + FKS_ID_LIMIT) | wrld->nfe;
    wrld->nfe = FKS_ID(ntt);
    return wrld->nttc--, true;
}

bool fks_entity_alive(const fks_world_t* wrld, fks_entity_t ntt) {
    if (!wrld_valid(wrld))      return false;
    if (!(ntt &= FKS_ID_MASK))  return FKS_LOG_NULL_INPUT("fks_entity_t"), false;
    
    fks_entity_t ntto = ntt;
    return (ntt <= wrld->ehwm) && !((ntto ^ (FKS_ESL(wrld, ntt))) & ~FKS_ID_MASK);
}

fks_component_t fks_component_register(fks_world_t* wrld, uint32_t size) {
    if (!wrld_valid(wrld))                  return FKS_NULL_COMPONENT;
    if (wrld->chwm == FKS_COMPONENT_MAX)    return FKS_LOG_GENERAL(FKS_ERROR, "Component limit reached"), FKS_NULL_COMPONENT;
    if (!(wrld->cdr[wrld->chwm + 1] = calloc(1, sizeof **wrld->cdr)))
        return FKS_LOG_ALLOC("fks_component_data*"), FKS_NULL_COMPONENT;

    fks_component_data_t* cmpd = wrld->cdr[wrld->chwm + 1];
    cmpd->dsc = FKS_COMPONENT_DENSE_ICAP;
    cmpd->scs = size;
    cmpd->ccs = true;

    if (!(cmpd->cds = calloc(cmpd->dsc, cmpd->scs)))
        return free(cmpd), wrld->cdr[wrld->chwm + 1] = NULL,
        FKS_LOG_ALLOC("void*"), FKS_NULL_COMPONENT;
    if (!(cmpd->rds = calloc(cmpd->dsc, sizeof *cmpd->rds)))
        return free(cmpd->cds), free(cmpd), wrld->cdr[wrld->chwm + 1] = NULL,
        FKS_LOG_ALLOC("fks_entity*"), FKS_NULL_COMPONENT;

    return ++wrld->chwm;
}

void fks_component_enable(fks_world_t* wrld, fks_component_t cmp) {
    if (wrld_cmp_valid(wrld, cmp)) wrld->cdr[cmp]->ccs = true;
}

void fks_component_disable(fks_world_t* wrld, fks_component_t cmp) {
    if (wrld_cmp_valid(wrld, cmp)) wrld->cdr[cmp]->ccs = false;
}

const fks_entity_t* fks_component_get_entities(const fks_world_t* wrld, fks_component_t cmp, uint32_t* rsize) {
    if (!rsize)                     return FKS_LOG_NULL_INPUT("uint32_t*"), NULL;
    if (!wrld_cmp_valid(wrld, cmp)) return NULL;

    *rsize = wrld->cdr[cmp]->nfs;
    return wrld->cdr[cmp]->rds;
}

bool fks_entity_add_component(fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp, const void* data) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp))    return false;
    if (!data)                                  return FKS_LOG_NULL_INPUT("void*"), false;
    
    fks_component_data_t* cmpd = wrld->cdr[cmp];
    if (!cmpd->ccs) return false;

    fks_css1** l1p; fks_css0** l0p; fks_entity_id_t* eidp;
    if (!*(l1p = &cmpd->css[FKS_CSS2_IDX(ntt)]) && !(*l1p = calloc(1, sizeof **l1p))) return FKS_LOG_ALLOC("fks_css1*"), false;
    if (!*(l0p =   &(**l1p)[FKS_CSS1_IDX(ntt)]) && !(*l0p = calloc(1, sizeof **l0p))) return FKS_LOG_ALLOC("fks_css0*"), false;
    if ( *(eidp =  &(**l0p)[FKS_CSS0_IDX(ntt)]))
        return FKS_LOG_GENERAL(FKS_WARNING, "Entity already contains component"), false;

    if (cmpd->nfs >= cmpd->dsc) {
        void* ncds; fks_entity_t* nrds;
        if (!(ncds = calloc(cmpd->dsc * 2, cmpd->scs)))         return FKS_LOG_ALLOC("void*"), false;
        if (!(nrds = calloc(cmpd->dsc * 2, sizeof *cmpd->rds))) return free(ncds), FKS_LOG_ALLOC("fks_entity_t*"), false;

        memcpy(ncds, cmpd->cds, cmpd->dsc * cmpd->scs);
        memcpy(nrds, cmpd->rds, cmpd->dsc * sizeof *cmpd->rds);
        
        free(cmpd->cds), free(cmpd->rds);
        cmpd->cds = ncds, cmpd->rds = nrds, cmpd->dsc *= 2;
    }

    memcpy((fks_uc*) cmpd->cds + (cmpd->nfs * cmpd->scs), data, cmpd->scs);
    cmpd->rds[cmpd->nfs] = ntt;
    *eidp = ++cmpd->nfs;

    return true;
}

bool fks_entity_remove_component(fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return false;
    
    fks_component_data_t* cmpd = wrld->cdr[cmp];
    fks_css1* l1; fks_css0* l0; fks_entity_id_t* eidp;
    if (!(l1 = cmpd->css[FKS_CSS2_IDX(ntt)]) || !(l0 = (*l1)[FKS_CSS1_IDX(ntt)]) || !*(eidp = &(*l0)[FKS_CSS0_IDX(ntt)])) return false;

    if (*eidp != cmpd->nfs) {
        memcpy(
            (fks_uc*) cmpd->cds + (*eidp - 1) * cmpd->scs,
            (fks_uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs,
            cmpd->scs
        );
        FKS_CSS(cmpd, cmpd->rds[cmpd->nfs - 1]) = *eidp;
        cmpd->rds[*eidp - 1] = cmpd->rds[cmpd->nfs - 1];
    }

    memset((fks_uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs, 0, cmpd->scs);
    cmpd->rds[--cmpd->nfs] = FKS_NULL_ENTITY;
    *eidp = FKS_ID(FKS_NULL_ENTITY);

    return true;
}

bool fks_entity_has_component(const fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return false;

    fks_component_data_t* cmpd = wrld->cdr[cmp];
    fks_css1* l1; fks_css0* l0;
    if (!(l1 = cmpd->css[FKS_CSS2_IDX(ntt)]) || !(l0 = (*l1)[FKS_CSS1_IDX(ntt)])) return false;
    
    return (*l0)[FKS_CSS0_IDX(ntt)];
}

void* fks_entity_get_component(const fks_world_t* wrld, fks_entity_t ntt, fks_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return NULL;
    
    fks_component_data_t* cmpd = wrld->cdr[cmp];
    fks_css1* l1; fks_css0* l0; fks_entity_id_t eid;
    if (!(l1 = cmpd->css[FKS_CSS2_IDX(ntt)]) || !(l0 = (*l1)[FKS_CSS1_IDX(ntt)]) || !(eid = (*l0)[FKS_CSS0_IDX(ntt)]))
        return FKS_LOG_GENERAL(FKS_WARNING, "Entity does not contain component"), NULL;

    return (fks_uc*) cmpd->cds + (eid - 1) * cmpd->scs;
}