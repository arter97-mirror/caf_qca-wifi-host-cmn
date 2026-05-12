/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 *
 */

/**
 * DOC: i_qdf_atomic64.h
 * This file provides OS dependent atomic64 APIs.
 */

#ifndef I_QDF_ATOMIC64_H
#define I_QDF_ATOMIC64_H

#include <qdf_status.h>         /* QDF_STATUS */
#include <linux/atomic.h>

typedef atomic64_t __qdf_atomic64_t;

/**
 * __qdf_atomic64_init() - initialize an atomic64 type variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: QDF_STATUS
 */
static inline QDF_STATUS __qdf_atomic64_init(__qdf_atomic64_t *v)
{
	atomic64_set(v, 0);
	return QDF_STATUS_SUCCESS;
}

/**
 * __qdf_atomic64_read() - read the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t __qdf_atomic64_read(__qdf_atomic64_t *v)
{
	return atomic64_read(v);
}

/**
 * __qdf_atomic64_inc() - increment the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void __qdf_atomic64_inc(__qdf_atomic64_t *v)
{
	atomic64_inc(v);
}

/**
 * __qdf_atomic64_dec() - decrement the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void __qdf_atomic64_dec(__qdf_atomic64_t *v)
{
	atomic64_dec(v);
}

/**
 * __qdf_atomic64_add() - add a value to the value of an atomic64 variable
 * @i: The amount by which to increase the atomic64 counter
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: None
 */
static inline void __qdf_atomic64_add(int64_t i, __qdf_atomic64_t *v)
{
	atomic64_add(i, v);
}

/**
 * __qdf_atomic64_sub() - Subtract a value from an atomic64 variable
 * @i: the amount by which to decrease the atomic64 counter
 * @v: a pointer to an opaque atomic64 variable
 *
 * Return: none
 */
static inline void __qdf_atomic64_sub(int64_t i, __qdf_atomic64_t *v)
{
	atomic64_sub(i, v);
}

/**
 * __qdf_atomic64_dec_and_test() - decrement an atomic64 variable and check
 * if the new value is zero
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return:
 * true (non-zero) if the new value is zero,
 * false (0) if the new value is non-zero
 */
static inline int __qdf_atomic64_dec_and_test(__qdf_atomic64_t *v)
{
	return atomic64_dec_and_test(v);
}

/**
 * __qdf_atomic64_set() - set a value to the value of an atomic64 variable
 * @v: A pointer to an opaque atomic64 variable
 * @i: value to assign
 *
 * Return: None
 */
static inline void __qdf_atomic64_set(__qdf_atomic64_t *v, int64_t i)
{
	atomic64_set(v, i);
}

/**
 * __qdf_atomic64_inc_return() - return the incremented value of an atomic64
 * variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t __qdf_atomic64_inc_return(__qdf_atomic64_t *v)
{
	return atomic64_inc_return(v);
}

/**
 * __qdf_atomic64_dec_return() - return the decremented value of an atomic64
 * variable
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: The current value of the variable
 */
static inline int64_t __qdf_atomic64_dec_return(__qdf_atomic64_t *v)
{
	return atomic64_dec_return(v);
}

/**
 * __qdf_atomic64_inc_not_zero() - increment if not zero
 * @v: A pointer to an opaque atomic64 variable
 *
 * Return: Returns non-zero on successful increment and zero otherwise
 */
static inline int64_t __qdf_atomic64_inc_not_zero(__qdf_atomic64_t *v)
{
	return atomic64_inc_not_zero(v);
}

#endif
