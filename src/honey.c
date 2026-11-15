#include "honey.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* =============== Entity ID =============== */
#define HNY_ID_BITS     (sizeof(hny_entity_id_t) * 8)
#define HNY_ID_LIMIT    (1ull << HNY_ID_BITS)
#define HNY_ID_MASK     (HNY_ID_LIMIT - 1)
#define HNY_ID(ntt)     ((hny_entity_id_t) ((ntt) & HNY_ID_MASK))

typedef uint32_t        hny_entity_id_t;

/* =============== Entity State List (ESL) | 2nd Level Paged List =============== */
#define HNY_ESL0_PAGE_BITS  12
#define HNY_ESL1_PAGE_BITS  20

#define HNY_ESL0_PAGE_SIZE  (1ull << HNY_ESL0_PAGE_BITS)
#define HNY_ESL1_PAGE_SIZE  (1ull << HNY_ESL1_PAGE_BITS)

#define HNY_ESL0_IDX(ntt)   ((HNY_ID(ntt))                         & (HNY_ESL0_PAGE_SIZE - 1))
#define HNY_ESL1_IDX(ntt)   (((HNY_ID(ntt)) >> HNY_ESL0_PAGE_BITS) & (HNY_ESL1_PAGE_SIZE - 1))
#define HNY_ESL(wrld, ntt)  ((*(wrld)->esl[HNY_ESL1_IDX(ntt)])[HNY_ESL0_IDX(ntt)])

#define HNY_ESL0_CALLOC(esl0ptr)    ((esl0ptr) = calloc(HNY_ESL0_PAGE_SIZE, sizeof **(esl0ptr)))

typedef hny_entity_t        hny_esl0[HNY_ESL0_PAGE_SIZE];
typedef hny_esl0*           hny_esl1[HNY_ESL1_PAGE_SIZE];

/* =============== Component Sparse Set (CSS) | 3rd Level Paged List  =============== */
#define HNY_CSS0_PAGE_BITS  10
#define HNY_CSS1_PAGE_BITS  11
#define HNY_CSS2_PAGE_BITS  11

#define HNY_CSS0_PAGE_SIZE  (1ull << HNY_CSS0_PAGE_BITS)
#define HNY_CSS1_PAGE_SIZE  (1ull << HNY_CSS1_PAGE_BITS)
#define HNY_CSS2_PAGE_SIZE  (1ull << HNY_CSS2_PAGE_BITS)

#define HNY_CSS0_IDX(ntt)   ((HNY_ID(ntt))                                                & (HNY_CSS0_PAGE_SIZE - 1))
#define HNY_CSS1_IDX(ntt)   (((HNY_ID(ntt)) >> HNY_CSS0_PAGE_BITS)                        & (HNY_CSS1_PAGE_SIZE - 1))
#define HNY_CSS2_IDX(ntt)   (((HNY_ID(ntt)) >> (HNY_CSS0_PAGE_BITS + HNY_CSS1_PAGE_BITS)) & (HNY_CSS2_PAGE_SIZE - 1))
#define HNY_CSS(cmpd, ntt)  ((*(*(cmpd)->css[HNY_CSS2_IDX(ntt)])[HNY_CSS1_IDX(ntt)])[HNY_CSS0_IDX(ntt)])

#define HNY_CSS0_CALLOC(css0ptr) ((css0ptr) = calloc(HNY_CSS0_PAGE_SIZE, sizeof **(css0ptr)))
#define HNY_CSS1_CALLOC(css1ptr) ((css1ptr) = calloc(HNY_CSS1_PAGE_SIZE, sizeof **(css1ptr)))

typedef hny_entity_id_t     hny_css0[HNY_CSS0_PAGE_SIZE];
typedef hny_css0*           hny_css1[HNY_CSS1_PAGE_SIZE];
typedef hny_css1*           hny_css2[HNY_CSS2_PAGE_SIZE];

/* =============== Component Data =============== */
#define HNY_COMPONENT_DENSE_ICAP    64

#define HNY_COMPONENT_LIMIT         (1ull << (sizeof(hny_component_t) * 8))
#define HNY_COMPONENT_MAX           (HNY_COMPONENT_LIMIT - 1)

/* =============== Error Logging =============== */
#define HNY_LOG_MAX_BUF_SIZE            128

#define HNY_LOG(loglv, errt, ...)       log_error(#loglv, errt, __func__, __LINE__, __VA_ARGS__)
#define HNY_LOG_GENERAL(loglv, msg)     HNY_LOG(loglv, "General", "%s", (msg))
#define HNY_LOG_BAD_INPUT(rec, type)    HNY_LOG(HNY_WARNING, "Input", "Bad input: %s for %s", (rec), (type))
#define HNY_LOG_NULL_INPUT(type)        HNY_LOG_BAD_INPUT("NULL", (type))
#define HNY_LOG_ALLOC(type)             HNY_LOG(HNY_ERROR, "Alloc", "Failed to allocate resource: %s", (type))

typedef enum {
    HNY_WARNING,
    HNY_ERROR
} hny_log_level;

/* =============== General ===============*/
typedef unsigned char hny_uc;

/* =============== Structs =============== */
typedef struct hny_component_data_t {
    hny_css2 css;        /**< Component Sparse Set     */
    hny_entity_t* rds;   /**< Reverse Dense Set        */
    void* cds;           /**< Component Dense Set      */

    hny_entity_id_t dsc; /**< Dense Set Capacity       */
    hny_entity_id_t nfs; /**< Next Free Slot           */
    uint32_t scs;        /**< Single Component Size    */
    bool ccs;            /**< Component Creation State */
} hny_component_data_t;

struct hny_world_t {
    hny_esl1 esl;                                    /**< Entity State List       */
    hny_component_data_t* cdr[HNY_COMPONENT_LIMIT];  /**< Component Data Registry */

    size_t nttc;                                     /**< Entity Count             */
    hny_entity_id_t nfe;                             /**< Next Free Entity         */
    hny_entity_id_t ehwm;                            /**< Entity High Watermark    */
    hny_component_t chwm;                            /**< Component High Watermark */
};

/* =============== Internal Funcs =============== */
static inline void log_error(const char* lv, const char* errt, const char* func, int line, const char* format, ...) {
    char buf[HNY_LOG_MAX_BUF_SIZE];
    va_list argv; va_start(argv, format);
    vsnprintf(buf, sizeof(buf), format, argv);
    va_end(argv);

    fprintf(stderr, "%s %s @ [%s:%d] - %s\n", lv, errt, func, line, buf);
}

static inline bool wrld_valid(const hny_world_t* wrld) {
    if (!wrld) return HNY_LOG_NULL_INPUT("hny_world_t*"), false;

    return true;
}

static inline bool wrld_ntt_valid(const hny_world_t* wrld, hny_entity_t ntt) {
    hny_entity_t ntto = ntt;
    if (!wrld_valid(wrld))                              return false;
    if (!(ntt &= HNY_ID_MASK))                          return HNY_LOG_NULL_INPUT("hny_entity_t"), false;
    if (ntt > wrld->ehwm)                               return HNY_LOG_BAD_INPUT("invalid entity", "hny_entity_t"), false;
    if ((ntto ^ (HNY_ESL(wrld, ntt))) & ~HNY_ID_MASK)   return HNY_LOG_BAD_INPUT("stale entity", "hny_entity_t"), false;

    return true;
}

static inline bool wrld_cmp_valid(const hny_world_t* wrld, hny_component_t cmp) {
    if (!wrld_valid(wrld))  return false;
    if (!cmp)               return HNY_LOG_NULL_INPUT("hny_component_t"), false;
    if (cmp > wrld->chwm)   return HNY_LOG_BAD_INPUT("invalid component", "hny_component_t"), false;

    return true;
}

static inline bool wrld_ntt_cmp_valid(const hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp) {
    return wrld_ntt_valid(wrld, ntt) && wrld_cmp_valid(wrld, cmp);
}

/* =============== Implementation =============== */
hny_world_t* hny_world_init(void) {
    hny_world_t* wrld = calloc(1, sizeof(hny_world_t));
    return wrld ? wrld : (HNY_LOG_ALLOC("hny_world_t*"), NULL);
}

void hny_world_fini(hny_world_t* wrld) {
    if (!wrld) return;

    for (size_t i = 0; i <= (wrld->ehwm >> HNY_ESL0_PAGE_BITS); ++i) free(wrld->esl[i]);
    
    hny_component_data_t* cmpd; hny_css1* tssl1;
    for (size_t cmp = 1; cmp <= wrld->chwm; ++cmp) {
        cmpd = wrld->cdr[cmp];
        for (size_t i = 0; i < HNY_CSS2_PAGE_SIZE; ++i) {
            if ((tssl1 = cmpd->css[i]))
                for (size_t j = 0; j < HNY_CSS1_PAGE_SIZE; ++j) free((*tssl1)[j]);
            free(tssl1);
        }
        free(cmpd->rds), free(cmpd->cds), free(cmpd);
    }

    free(wrld);
}

size_t hny_world_entity_count(const hny_world_t* wrld) {
    return wrld_valid(wrld) ? wrld->nttc : SIZE_MAX;
}

hny_entity_t hny_entity_create(hny_world_t* wrld) {
    if (!wrld_valid(wrld)) return HNY_NULL_ENTITY;

    if (wrld->nfe) {
        hny_entity_id_t _nfe = wrld->nfe;
        wrld->nttc++;
        wrld->nfe = HNY_ID(HNY_ESL(wrld, _nfe));
        
        return (HNY_ESL(wrld, _nfe) & ~HNY_ID_MASK) | _nfe;
    } else {
        if (wrld->ehwm == HNY_ID_MASK) return HNY_LOG_GENERAL(HNY_ERROR, "Entity limit reached"), HNY_NULL_ENTITY;
        if (!(wrld->ehwm && ((wrld->ehwm + 1) & (HNY_ESL0_PAGE_SIZE - 1))) && !HNY_ESL0_CALLOC(wrld->esl[HNY_ESL1_IDX(wrld->ehwm + 1)]))
            return HNY_LOG_ALLOC("hny_esl0*"), HNY_NULL_ENTITY;

        return wrld->nttc++, ++wrld->ehwm;
    }
}

bool hny_entity_destroy(hny_world_t* wrld, hny_entity_t ntt) {
    if (!wrld_ntt_valid(wrld, ntt)) return false;

    for (size_t cmp = 1; cmp <= wrld->chwm; ++cmp) hny_entity_remove_component(wrld, ntt, (hny_component_t) cmp);
    HNY_ESL(wrld, ntt) = ((HNY_ESL(wrld, ntt) & ~HNY_ID_MASK) + HNY_ID_LIMIT) | wrld->nfe;
    wrld->nfe = HNY_ID(ntt);
    return wrld->nttc--, true;
}

bool hny_entity_alive(const hny_world_t* wrld, hny_entity_t ntt) {
    if (!wrld_valid(wrld))      return false;
    hny_entity_t ntto = ntt;
    if (!(ntt &= HNY_ID_MASK))  return false;
    
    return (ntt <= wrld->ehwm) && !((ntto ^ (HNY_ESL(wrld, ntt))) & ~HNY_ID_MASK);
}

hny_component_t hny_component_register(hny_world_t* wrld, uint32_t size) {
    if (!wrld_valid(wrld))                  return HNY_NULL_COMPONENT;
    if (!size)                              return HNY_LOG_NULL_INPUT("uint32_t"), HNY_NULL_COMPONENT;
    if (wrld->chwm == HNY_COMPONENT_MAX)    return HNY_LOG_GENERAL(HNY_ERROR, "Component limit reached"), HNY_NULL_COMPONENT;
    if (!(wrld->cdr[wrld->chwm + 1] = calloc(1, sizeof **wrld->cdr)))
        return HNY_LOG_ALLOC("hny_component_data*"), HNY_NULL_COMPONENT;

    hny_component_data_t* cmpd = wrld->cdr[wrld->chwm + 1];
    cmpd->dsc = HNY_COMPONENT_DENSE_ICAP;
    cmpd->scs = size;
    cmpd->ccs = true;

    if (!(cmpd->cds = calloc(cmpd->dsc, cmpd->scs)))
        return free(cmpd), wrld->cdr[wrld->chwm + 1] = NULL,
        HNY_LOG_ALLOC("void*"), HNY_NULL_COMPONENT;
    if (!(cmpd->rds = calloc(cmpd->dsc, sizeof *cmpd->rds)))
        return free(cmpd->cds), free(cmpd), wrld->cdr[wrld->chwm + 1] = NULL,
        HNY_LOG_ALLOC("hny_entity*"), HNY_NULL_COMPONENT;

    return ++wrld->chwm;
}

void hny_component_enable(hny_world_t* wrld, hny_component_t cmp) {
    if (wrld_cmp_valid(wrld, cmp)) wrld->cdr[cmp]->ccs = true;
}

void hny_component_disable(hny_world_t* wrld, hny_component_t cmp) {
    if (wrld_cmp_valid(wrld, cmp)) wrld->cdr[cmp]->ccs = false;
}

const hny_entity_t* hny_component_get_entities(const hny_world_t* wrld, hny_component_t cmp, size_t* rsize) {
    if (!rsize)                     return HNY_LOG_NULL_INPUT("uint32_t*"), NULL;
    if (!wrld_cmp_valid(wrld, cmp)) return NULL;

    *rsize = wrld->cdr[cmp]->nfs;
    return wrld->cdr[cmp]->rds;
}

bool hny_entity_add_component(hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp, const void* data) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp))    return false;
    if (!data)                                  return HNY_LOG_NULL_INPUT("void*"), false;
    
    hny_component_data_t* cmpd = wrld->cdr[cmp];
    if (!cmpd->ccs) return false;

    hny_css1** l1p; hny_css0** l0p; hny_entity_id_t* eidp;
    if (!*(l1p = &cmpd->css[HNY_CSS2_IDX(ntt)]) && !HNY_CSS1_CALLOC(*l1p)) return HNY_LOG_ALLOC("hny_css1*"), false;
    if (!*(l0p =   &(**l1p)[HNY_CSS1_IDX(ntt)]) && !HNY_CSS0_CALLOC(*l0p)) return HNY_LOG_ALLOC("hny_css0*"), false;
    if ( *(eidp =  &(**l0p)[HNY_CSS0_IDX(ntt)]))
        return HNY_LOG_GENERAL(HNY_WARNING, "Entity already contains component"), false;

    void *fcds = NULL, *frds = NULL;
    if (cmpd->nfs >= cmpd->dsc) {
        void* _cds; hny_entity_t* _rds;
        if (cmpd->dsc > HNY_ID_LIMIT / 2)
            return HNY_LOG_GENERAL(HNY_ERROR, "component capacity limit reached"), false;
        hny_entity_id_t _dsc = cmpd->dsc * 2;
        if (!(_cds = calloc(_dsc, cmpd->scs)))         return HNY_LOG_ALLOC("void*"), false;
        if (!(_rds = calloc(_dsc, sizeof *cmpd->rds))) return free(_cds), HNY_LOG_ALLOC("hny_entity_t*"), false;

        memcpy(_cds, cmpd->cds, (size_t) cmpd->dsc * cmpd->scs);
        memcpy(_rds, cmpd->rds, (size_t) cmpd->dsc * sizeof *cmpd->rds);
        
        fcds = cmpd->cds, frds = cmpd->rds;
        cmpd->cds = _cds, cmpd->rds = _rds, cmpd->dsc = _dsc;
    }

    memcpy((hny_uc*) cmpd->cds + (size_t) cmpd->nfs * cmpd->scs, data, cmpd->scs);
    free(fcds), free(frds);
    cmpd->rds[cmpd->nfs] = ntt;
    *eidp = ++cmpd->nfs;

    return true;
}

bool hny_entity_remove_component(hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return false;
    
    hny_component_data_t* cmpd = wrld->cdr[cmp];
    hny_css1* l1; hny_css0* l0; hny_entity_id_t* eidp;
    if (!(l1 = cmpd->css[HNY_CSS2_IDX(ntt)]) || !(l0 = (*l1)[HNY_CSS1_IDX(ntt)]) || !*(eidp = &(*l0)[HNY_CSS0_IDX(ntt)])) return false;

    if (*eidp != cmpd->nfs) {
        memcpy(
            (hny_uc*) cmpd->cds + (*eidp - 1) * cmpd->scs,
            (hny_uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs,
            cmpd->scs
        );
        HNY_CSS(cmpd, cmpd->rds[cmpd->nfs - 1]) = *eidp;
        cmpd->rds[*eidp - 1] = cmpd->rds[cmpd->nfs - 1];
    }

    memset((hny_uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs, 0, cmpd->scs);
    cmpd->rds[--cmpd->nfs] = HNY_NULL_ENTITY;
    *eidp = HNY_ID(HNY_NULL_ENTITY);

    return true;
}

bool hny_entity_has_component(const hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return false;

    hny_component_data_t* cmpd = wrld->cdr[cmp];
    hny_css1* l1; hny_css0* l0;
    if (!(l1 = cmpd->css[HNY_CSS2_IDX(ntt)]) || !(l0 = (*l1)[HNY_CSS1_IDX(ntt)])) return false;
    
    return (*l0)[HNY_CSS0_IDX(ntt)];
}

void* hny_entity_get_component(const hny_world_t* wrld, hny_entity_t ntt, hny_component_t cmp) {
    if (!wrld_ntt_cmp_valid(wrld, ntt, cmp)) return NULL;
    
    hny_component_data_t* cmpd = wrld->cdr[cmp];
    hny_css1* l1; hny_css0* l0; hny_entity_id_t eid;
    if (!(l1 = cmpd->css[HNY_CSS2_IDX(ntt)]) || !(l0 = (*l1)[HNY_CSS1_IDX(ntt)]) || !(eid = (*l0)[HNY_CSS0_IDX(ntt)]))
        return NULL;

    return (hny_uc*) cmpd->cds + (eid - 1) * cmpd->scs;
}