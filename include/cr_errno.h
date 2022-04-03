/* 协程层面的错误号 */
#ifndef __CR_ERRNO_H__
#define __CR_ERRNO_H__


/* 具体错误号的值 */
typedef enum {
    CR_ERR_OK = 0,      /* 没有错误 */
    CR_ERR_FAIL,        /* 未确定原因失败 */
    CR_ERR_INVAL,       /* 无效参数 */
    CR_ERR_INTR,        /* 等待过程被外部中断 */
    CR_ERR_CLOSE,       /* 等待的资源被关闭 */
    CR_ERR_EXPR,        /* 等待超时 */
    CR_ERR_MAX,
} cr_errno_enum;


#endif /* __CR_ERRNO_H__ */
