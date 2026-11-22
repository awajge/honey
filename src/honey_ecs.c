#include "honey_ecs.h"
#include "internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#define HNY_STATIC_ASSERT(name, expr) typedef char name[(expr) ? 1 : -1];

/* =============== Entity ID =============== */
typedef u32             hny_entity_id_t;

#define HNY_ID_BITS     (sizeof(hny_entity_id_t) * CHAR_BIT)
#define HNY_ID_LIMIT    (1ull << HNY_ID_BITS)
#define HNY_ID_MASK     (HNY_ID_LIMIT - 1)
#define HNY_ID(ntt)     ((hny_entity_id_t) ((ntt) & HNY_ID_MASK))

/* =============== Entity State List (ESL) | 2nd Level Paged List of IDS =============== */
#define HNY_ESL_BITSPACE    HNY_ID_BITS

#define HNY_ESL0_PAGE_BITS  12
#define HNY_ESL1_PAGE_BITS  20

HNY_STATIC_ASSERT(hny_esl0_page_bits_g0_lbp, HNY_ESL0_PAGE_BITS > 0 && HNY_ESL0_PAGE_BITS < HNY_ID_BITS)
HNY_STATIC_ASSERT(hny_esl1_page_bits_g0_lbp, HNY_ESL1_PAGE_BITS > 0 && HNY_ESL1_PAGE_BITS < HNY_ID_BITS)
HNY_STATIC_ASSERT(hny_esl_page_bits_ebp, HNY_ESL0_PAGE_BITS + HNY_ESL1_PAGE_BITS == HNY_ESL_BITSPACE)

#define HNY_ESL0_PAGE_SIZE  (1ull << HNY_ESL0_PAGE_BITS)
#define HNY_ESL1_PAGE_SIZE  (1ull << HNY_ESL1_PAGE_BITS)

typedef hny_entity_t        hny_esl0[HNY_ESL0_PAGE_SIZE];
typedef hny_esl0*           hny_esl1[HNY_ESL1_PAGE_SIZE];

#define HNY_ESL0_IDX(ntt)   ((HNY_ID(ntt))                         & (HNY_ESL0_PAGE_SIZE - 1))
#define HNY_ESL1_IDX(ntt)   (((HNY_ID(ntt)) >> HNY_ESL0_PAGE_BITS) & (HNY_ESL1_PAGE_SIZE - 1))
#define HNY_ESL(wrld, ntt)  ((*(wrld)->esl[HNY_ESL1_IDX(ntt)])[HNY_ESL0_IDX(ntt)])

#define HNY_ESL0_CALLOC(esl0ptr)    ((esl0ptr) = calloc(HNY_ESL0_PAGE_SIZE, sizeof **(esl0ptr)))

/* =============== Component Sparse Set (CSS) | 3rd Level Paged List of IDS  =============== */
#define HNY_CSS_BITSPACE    HNY_ID_BITS

#define HNY_CSS0_PAGE_BITS  10
#define HNY_CSS1_PAGE_BITS  11
#define HNY_CSS2_PAGE_BITS  11

HNY_STATIC_ASSERT(hny_css0_page_bits_g0_lbp, HNY_CSS0_PAGE_BITS > 0 && HNY_CSS0_PAGE_BITS < HNY_CSS_BITSPACE)
HNY_STATIC_ASSERT(hny_css1_page_bits_g0_lbp, HNY_CSS1_PAGE_BITS > 0 && HNY_CSS1_PAGE_BITS < HNY_CSS_BITSPACE)
HNY_STATIC_ASSERT(hny_css2_page_bits_g0_lbp, HNY_CSS2_PAGE_BITS > 0 && HNY_CSS2_PAGE_BITS < HNY_CSS_BITSPACE)
HNY_STATIC_ASSERT(hny_css_page_bits_ebp, HNY_CSS0_PAGE_BITS + HNY_CSS1_PAGE_BITS + HNY_CSS2_PAGE_BITS == HNY_CSS_BITSPACE)

#define HNY_CSS0_PAGE_SIZE  (1ull << HNY_CSS0_PAGE_BITS)
#define HNY_CSS1_PAGE_SIZE  (1ull << HNY_CSS1_PAGE_BITS)
#define HNY_CSS2_PAGE_SIZE  (1ull << HNY_CSS2_PAGE_BITS)

typedef hny_entity_id_t     hny_css0[HNY_CSS0_PAGE_SIZE];
typedef hny_css0*           hny_css1[HNY_CSS1_PAGE_SIZE];
typedef hny_css1*           hny_css2[HNY_CSS2_PAGE_SIZE];

#define HNY_CSS0_IDX(ntt)   ((HNY_ID(ntt))                                                & (HNY_CSS0_PAGE_SIZE - 1))
#define HNY_CSS1_IDX(ntt)   (((HNY_ID(ntt)) >> HNY_CSS0_PAGE_BITS)                        & (HNY_CSS1_PAGE_SIZE - 1))
#define HNY_CSS2_IDX(ntt)   (((HNY_ID(ntt)) >> (HNY_CSS0_PAGE_BITS + HNY_CSS1_PAGE_BITS)) & (HNY_CSS2_PAGE_SIZE - 1))
#define HNY_CSS(cmpd, ntt)  ((*(*(cmpd)->css[HNY_CSS2_IDX(ntt)])[HNY_CSS1_IDX(ntt)])[HNY_CSS0_IDX(ntt)])

#define HNY_CSS0_CALLOC(css0ptr) ((css0ptr) = calloc(HNY_CSS0_PAGE_SIZE, sizeof **(css0ptr)))
#define HNY_CSS1_CALLOC(css1ptr) ((css1ptr) = calloc(HNY_CSS1_PAGE_SIZE, sizeof **(css1ptr)))

/* =============== Component Data =============== */
#define HNY_COMPONENT_DENSE_ICAP    64 // no static assert upper bound - assign reasonably

HNY_STATIC_ASSERT(hny_cmp_dense_icap_g0, HNY_COMPONENT_DENSE_ICAP > 0)

#define HNY_COMPONENT_LIMIT         (1ull << (sizeof(hny_component_t) * CHAR_BIT))
#define HNY_COMPONENT_MAX           (HNY_COMPONENT_LIMIT - 1)

/* =============== Error Logging =============== */
#define HNY_LOG_MAX_BUF_SIZE        128 // no static assert upper bound - assign reasonably

HNY_STATIC_ASSERT(hny_log_max_buf_size_g0, HNY_LOG_MAX_BUF_SIZE > 0)

// ===== General Logging
#define HNY_LOG(loglv, errt, ...)       log_error(#loglv, errt, __func__, __LINE__, __VA_ARGS__)
#define HNY_LOG_GENERAL(loglv, msg)     HNY_LOG(loglv, "General", "%s", (msg))

// ===== FATALS

// ===== ERRORS
#define HNY_LOG_ALLOC(type)             HNY_LOG(HNY_LOG_ERROR, "Alloc", "Failed to allocate resource: %s", (type))

// ===== WARNINGS
#define HNY_LOG_BAD_INPUT(rec, type)    HNY_LOG(HNY_LOG_WARNING, "Input", "Bad input: %s for %s", (rec), (type))
#define HNY_LOG_NULL_INPUT(type)        HNY_LOG_BAD_INPUT("NULL", (type))

typedef enum hny_log_t {
    HNY_LOG_WARNING,
    HNY_LOG_ERROR,
    HNY_LOG_FATAL
} hny_log_t;

/* =============== Structs =============== */
typedef struct hny_component_data_t {
    hny_css2 css;           /**< Component Sparse Set     */
    hny_entity_t* rds;      /**< Reverse Dense Set        */
    void* cds;              /**< Component Dense Set      */

    size_t scs;             /**< Single Component Size    */
    hny_entity_id_t dsc;    /**< Dense Set Capacity       */
    hny_entity_id_t nfs;    /**< Next Free Slot           */
    bool ccs;               /**< Component Creation State */
} hny_component_data_t;

struct hny_world_t {
    hny_esl1 esl;                                   /**< Entity State List       */
    hny_component_data_t* cdr[HNY_COMPONENT_LIMIT]; /**< Component Data Registry */

    size_t nttc;                                    /**< Entity Count             */
    hny_entity_id_t nfe;                            /**< Next Free Entity         */
    hny_entity_id_t ehwm;                           /**< Entity High Watermark    */
    hny_component_t chwm;                           /**< Component High Watermark */
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
    for (size_t c = 1; c <= wrld->chwm; ++c) {
        cmpd = wrld->cdr[c];
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
        if (wrld->ehwm == HNY_ID_MASK) return HNY_LOG_GENERAL(HNY_LOG_ERROR, "Entity limit reached"), HNY_NULL_ENTITY;
        if (!(wrld->ehwm && ((wrld->ehwm + 1) & (HNY_ESL0_PAGE_SIZE - 1))) && !HNY_ESL0_CALLOC(wrld->esl[HNY_ESL1_IDX(wrld->ehwm + 1)]))
            return HNY_LOG_ALLOC("hny_esl0*"), HNY_NULL_ENTITY;

        return wrld->nttc++, ++wrld->ehwm;
    }
}

bool hny_entity_destroy(hny_world_t* wrld, hny_entity_t ntt) {
    if (!wrld_ntt_valid(wrld, ntt)) return false;

    for (size_t c = 1; c <= wrld->chwm; ++c) hny_entity_remove_component(wrld, ntt, (hny_component_t) c);
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
    if (!size)                              return HNY_LOG_NULL_INPUT("size_t"), HNY_NULL_COMPONENT;
    if (wrld->chwm == HNY_COMPONENT_MAX)    return HNY_LOG_GENERAL(HNY_LOG_ERROR, "Component limit reached"), HNY_NULL_COMPONENT;
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

const hny_entity_t* hny_component_get_entities(const hny_world_t* wrld,  hny_component_t cmp, size_t* rsize) {
    if (!rsize)                     return HNY_LOG_NULL_INPUT("size_t*"), NULL;
    if (!wrld_cmp_valid(wrld, cmp)) return NULL;

    *rsize = wrld->cdr[cmp]->nfs;
    return wrld->cdr[cmp]->rds;
}

hny_entity_t* hny_components_get_entities(const hny_world_t* wrld, const hny_component_t* cmps, size_t ncmps, size_t* rsize) {
    if (!wrld_valid(wrld))              return NULL;
    if (cmps == NULL)                   return HNY_LOG_NULL_INPUT("hny_component_t*"), NULL;
    if (rsize == NULL)                  return HNY_LOG_NULL_INPUT("size_t"), NULL;
    if (ncmps < 1)                      return HNY_LOG_BAD_INPUT("<1", "size_t"), NULL;
    if (!wrld_cmp_valid(wrld, cmps[0])) return HNY_LOG_BAD_INPUT("bad component", "hny_component_t"), NULL;

    hny_component_data_t* lec = wrld->cdr[cmps[0]];
    for (size_t c = 1; c < ncmps; ++c) {
        if (!wrld_cmp_valid(wrld, cmps[c])) return HNY_LOG_BAD_INPUT("bad component", "hny_component_t"), NULL;
        if (wrld->cdr[cmps[c]]->nfs < lec->nfs) lec = wrld->cdr[cmps[c]];
    }

    *rsize = 0;
    hny_entity_t* qcmps = malloc((lec->nfs ? lec->nfs : 1) * sizeof(hny_entity_t));
    if (qcmps == NULL) return HNY_LOG_ALLOC("hny_entity_t*"), NULL;
    for (size_t i = 0; i < lec->nfs; ++i) {
        for (size_t j = 0; j < ncmps; ++j)
            if (wrld->cdr[cmps[j]] != lec && !HNY_CSS(wrld->cdr[cmps[j]], lec->rds[i])) goto _;
        qcmps[(*rsize)++] = lec->rds[i];
        _: continue;
    }

    return qcmps;
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
        return HNY_LOG_GENERAL(HNY_LOG_WARNING, "Entity already contains component"), false;

    void *fcds = NULL, *frds = NULL;
    if (cmpd->nfs >= cmpd->dsc) {
        void* _cds; hny_entity_t* _rds;
        if (cmpd->dsc > HNY_ID_LIMIT / 2)
            return HNY_LOG_GENERAL(HNY_LOG_ERROR, "component capacity limit reached"), false;
        hny_entity_id_t _dsc = cmpd->dsc * 2;
        if (!(_cds = calloc(_dsc, cmpd->scs)))         return HNY_LOG_ALLOC("void*"), false;
        if (!(_rds = calloc(_dsc, sizeof *cmpd->rds))) return free(_cds), HNY_LOG_ALLOC("hny_entity_t*"), false;

        memcpy(_cds, cmpd->cds, (size_t) cmpd->dsc * cmpd->scs);
        memcpy(_rds, cmpd->rds, (size_t) cmpd->dsc * sizeof *cmpd->rds);
        
        fcds = cmpd->cds, frds = cmpd->rds;
        cmpd->cds = _cds, cmpd->rds = _rds, cmpd->dsc = _dsc;
    }

    memcpy((uc*) cmpd->cds + (size_t) cmpd->nfs * cmpd->scs, data, cmpd->scs);
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
            (uc*) cmpd->cds + (*eidp - 1) * cmpd->scs,
            (uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs,
            cmpd->scs
        );
        HNY_CSS(cmpd, cmpd->rds[cmpd->nfs - 1]) = *eidp;
        cmpd->rds[*eidp - 1] = cmpd->rds[cmpd->nfs - 1];
    }

    memset((uc*) cmpd->cds + (cmpd->nfs - 1) * cmpd->scs, 0, cmpd->scs);
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

    return (uc*) cmpd->cds + (eid - 1) * cmpd->scs;
}