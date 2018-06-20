// Author(s): David N. Jansen, Radboud Universiteit, Nijmegen, The Netherlands
//
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file lts/detail/coroutine.h
///
/// \brief helper macros for coroutines
///
/// \details The following macros are intended to be used for **coroutines**
/// that are executed in _lockstep,_ i. e. both routines do (approximately) the
/// same amount of work until one of them terminates successfully.  The macros
/// are suitable for coroutines that are member methods of a class.
///
/// First, the coroutine has to be _declared_ (if it is a class method, the
/// declaration is in the body of the class).  One writes:
///
///     DECLARE_COROUTINE(function name, formal parameters,
///                 local variables, shared data type, shared variable name,
///                 interrupt locations);
///
/// Later (outside the class body), one can _define_ the coroutine, i. e. give
/// its function body.  This is written as:
///
///     DEFINE_COROUTINE(namespace, function name, formal parameters,
///                 local variables, shared data type, shared variable name,
///                 interrupt locations)
///     {
///         function body;
///     } END_COROUTINE
///
/// The  declaration  and  definition  must  have  the  same  parameter
/// list  (except  for  the  namespace).    The  formal  parameters  and
/// the  local  variables  are  given  as  a  so-called  boost  sequence:
/// `((type1, var1)) ((type2, var2)) ((type3, var3))`  etc.    In  the
/// coroutine,  parameters  and  variables  can  be  accessed  normally.
/// The  interrupt  locations  are  a  similar  (but  simpler)  boost
/// sequence:  `(location1) (location2) (location3)`  etc.
/// The macros assume that there is at least one parameter, at least one
/// local variable and at least one interrupt location.
///
/// Local variables also must be declared/defined using the macro because if
/// one coroutine is interrupted, its local variables have to be stored
/// temporarily until it resumes.
///
/// There is one shared parameter for communication between the coroutines.  If
/// one needs more than one shared parameter, a class or struct holding all
/// shared parameters has to be defined.  Both coroutines will access the same
/// shared variable.
///
/// Use the `namespace` parameter in the coroutine definition to indicate the
/// name of the class of which it is part. Include a final `::`.
///
/// Interrupt locations are places where a unit of work is counted and the
/// coroutine may be interrupted (to allow the other coroutine to work).  As
/// code without loops always incurs the same time complexity, only loop
/// statements need to be counted.  There are three commands to create a loop
/// whose iterations are counted, closely corresponding to ``normal'' loops:
///
///     COROUTINE_WHILE(interrupt location, condition)  while (condition)
///     {                                               {
///         loop body;                                      loop body;
///     }                                               }
///     END_COROUTINE_WHILE;
///
///     COROUTINE_FOR(interrupt location,               for (initialisation;
///             initialisation, condition, update)          condition; update)
///     {                                               {
///         loop body;                                      loop body;
///     }                                               }
///     END_COROUTINE_FOR;
///
///     COROUTINE_DO_WHILE(interrupt locatn,condition)  do
///     {                                               {
///         loop body;                                      loop body;
///     }                                               }
///     END_COROUTINE_DO_WHILE;                         while (condition);
///
/// These macros hide some code that counts how many iterations have been
/// executed in a loop at the *end* of each iteration;  if one coroutine has
/// done enough work, it is interrupted to let the other catch up.  It is
/// assumed that a coroutine does at most `SIZE_MAX + 1` units of work.
///
/// A coroutine may also terminate explicitly by executing the statement
/// `TERMINATE_COROUTINE_SUCCESSFULLY();`.  If it should no longer execute but
/// has not terminated successfully, it may call `ABORT_THIS_COROUTINE();`.
/// If it does no longer allow the other coroutine to run up, it may indicate
/// so by calling `ABORT_OTHER_COROUTINE();`.
///
/// A pair of coroutines is called using the macro `RUN_COROUTINES`.  This
/// macro takes two coroutine names, two actual parameter lists, and two
/// ``final'' statements:
///
///     RUN_COROUTINES(function name 1, actual parameters, final statement 1,
///                    function name 2, actual parameters, final statement 2,
///                    shared data type, shared data initialiser);
///
/// It initialises the shared data with the indicated initialiser, given
/// as boost sequence `(initialiser1) (initialiser2) (initialiser3)` etc.;
/// then, it calls the two coroutines with the respective actual parameters,
/// given as `(param1) (param2) (param3)` etc., and makes sure that they do
/// approximately the same amount of work.  As soon as the first one finishes,
/// its associated final statement is executed and `RUN_COROUTINES` terminates.
///
/// ``Approximately the same amount of work'' is defined as:  The difference
/// between the number of steps taken by the two coroutines (i. e. the balance
/// of work) always is between -1 and +1.
///
/// \author David N. Jansen, Radboud Universiteit, Nijmegen, The Netherlands

#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <cstdlib>       // for std::size_t
#include <cassert>
#include <boost/preprocessor.hpp>





/* ************************************************************************* */
/*                                                                           */
/*                             I N T E R N A L S                             */
/*                                                                           */
/* ************************************************************************* */





namespace mcrl2
{
namespace lts
{
namespace detail
{
namespace coroutine
{

/// \brief type to indicate what to do with the coroutine
/// \details When a coroutine is interrupted, this type indicates what the
/// interrupted coroutine means to do:
/// - `_coroutine_CONTINUE` indicates that the other coroutine may catch up
///   but the interrupted one would like to continue later.
/// - `_coroutine_TERMINATE` indicates that the coroutine has terminated
///   successfully.
/// - `_coroutine_ABORT` indicates that the coroutine is aborted.  The other
///   coroutine should be allowed to finish without further interruptions.
typedef enum { _coroutine_CONTINUE, _coroutine_TERMINATE, _coroutine_ABORT }
                                                        _coroutine_result_t;

} // end namespace coroutine
} // end namespace detail
} // end namespace lts
} // end namespace mcrl2

/// _coroutine_VARDEF is an internal macro to define structure members for
/// variables (parameters or locals).
#define _coroutine_VARDEF(varseq)                                             \
                    BOOST_PP_SEQ_FOR_EACH(_coroutine_VARDEF_1, , varseq)
#define _coroutine_VARDEF_1(r,data,elem) _coroutine_VARDEF_2 elem
#define _coroutine_VARDEF_2(t1,v1)                                            \
                    t1 BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, v1), _var);

/// _coroutine_PARLST is an internal macro to define a parameter list for the
/// constructor.
#define _coroutine_PARLST(varseq)                                             \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(_coroutine_PARLST_1, , varseq))
#define _coroutine_PARLST_1(s,data,elem) _coroutine_PARLST_2 elem
#define _coroutine_PARLST_2(t1,v1)                                            \
                    t1 BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, v1), _new)

/// _coroutine_INITLST is an internal macro to define a member initializer list
/// for the constructor.
#define _coroutine_INITLST(varseq)                                            \
                    BOOST_PP_SEQ_FOR_EACH(_coroutine_INITLST_1, , varseq)
#define _coroutine_INITLST_1(r,data,elem) _coroutine_INITLST_2 elem
#define _coroutine_INITLST_2(t1,v1)                                           \
                    BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, v1), _var)         \
                        (BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, v1), _new)),

/// _coroutine_ALIAS is an internal macro to define aliases for parameters and
/// local variables, so that the programmer can access them easily.
#define _coroutine_ALIAS(varseq)                                              \
                    BOOST_PP_SEQ_FOR_EACH(_coroutine_ALIAS_1, , varseq)
#define _coroutine_ALIAS_1(r,data,elem) _coroutine_ALIAS_2 elem
#define _coroutine_ALIAS_2(t1,v1)                                             \
                    t1& v1 = BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_param.      \
                                                    _coroutine_, v1), _var);

/// _coroutine_ENUMDEF is an internal macro to define an appropriate
/// enumeration type for interrupt locations in a coroutine.
#define _coroutine_ENUMDEF(lblseq)                                            \
                    BOOST_PP_SEQ_FOR_EACH(_coroutine_ENUMDEF_1, , lblseq)
#define _coroutine_ENUMDEF_1(r,data,label)                                    \
                    , BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, label), _enum)

/// _coroutine_SWITCHCASE is an internal macro to jump to the interrupt
/// location where the coroutine was interrupted.
#define _coroutine_SWITCHCASE(lblseq)                                         \
                    BOOST_PP_SEQ_FOR_EACH(_coroutine_SWITCHCASE_1, , lblseq)
#define _coroutine_SWITCHCASE_1(r,data,label)                                 \
            case BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, label), _enum):       \
                goto BOOST_PP_CAT(BOOST_PP_CAT(_coroutine_, label), _label);





/* ************************************************************************* */
/*                                                                           */
/*                    E X T E R N A L   I N T E R F A C E                    */
/*                                                                           */
/* ************************************************************************* */





/// \brief declare a member method or a function as a coroutine
/// \details A coroutine should first be _declared_ and then _defined._  For
/// the latter, see the macro `DEFINE_COROUTINE`.
/// \param routine     name of the coroutine
/// \param param       formal parameter list, as a boost sequence:
///                    `((type1, var1)) ((type2, var2)) ((type3, var3))` etc.
/// \param local       local variables, as a boost sequence:
///                    `((type1, var1)) ((type2, var2)) ((type3, var3))` etc.
/// \param shared_type type of data shared between the two coroutines
/// \param shared_var  name of the formal parameter that contains the shared
///                    data
/// \param locations   locations where the coroutine can be interrupted, as a
///                    boost sequence: `(location1) (location2) (location3)`
///                    etc.
#define DECLARE_COROUTINE(routine, param, local, shared_type, shared_var,     \
                                                                    locations)\
    enum _coroutine_ ## routine ## _location                                  \
    {                                                                         \
         _coroutine_BEGIN_ ## routine ## _enum = __LINE__ /* no comma here */ \
         _coroutine_ENUMDEF(locations)                                        \
    };                                                                        \
                                                                              \
    class _coroutine_ ## routine ## _struct                                   \
    {                                                                         \
      public:                                                                 \
         _coroutine_VARDEF(param)                                             \
         enum _coroutine_ ## routine ## _location _coroutine_location;        \
         _coroutine_VARDEF(local)                                             \
         /* constructor: */                                                   \
         _coroutine_ ## routine ## _struct(_coroutine_PARLST(param))          \
           : _coroutine_INITLST(param) /* no comma here */                    \
             _coroutine_location(_coroutine_BEGIN_ ## routine ## _enum)       \
        {  }                                                                  \
    };                                                                        \
                                                                              \
    inline coroutine::_coroutine_result_t _coroutine_ ## routine ## _func(    \
                          std::size_t _coroutine_allowance,                   \
                          _coroutine_ ## routine ## _struct& _coroutine_param,\
                          shared_type& shared_var);


/// \brief define a member method or a function as a coroutine
/// \details A coroutine should first be _declared_ and then _defined._  For
/// the former, see the macro `DECLARE_COROUTINE`.
/// \param namespace   namespace of the coroutine.  If it is a member method,
///                    use `class name::`.
/// \param routine     name of the coroutine
/// \param param       formal parameter list, as a boost sequence:
///                    `((type1, var1)) ((type2, var2)) ((type3, var3))` etc.
/// \param local       local variables, as a boost sequence:
///                    `((type1, var1)) ((type2, var2)) ((type3, var3))` etc.
/// \param shared_type type of data shared between the two coroutines
/// \param shared_var  name of the formal parameter that contains the shared
///                    data
/// \param locations   locations where the coroutine can be interrupted, as a
///                    boost sequence: `(location1) (location2) (location3)`
///                    etc.
#define DEFINE_COROUTINE(namespace, routine, param, local, shared_type,       \
                                                        shared_var, locations)\
coroutine::_coroutine_result_t namespace _coroutine_ ## routine ## _func(     \
       std::size_t _coroutine_allowance,                                      \
       typename namespace _coroutine_ ## routine ## _struct& _coroutine_param,\
       shared_type& shared_var)                                               \
{                                                                             \
    _coroutine_ALIAS(param)                                                   \
    _coroutine_ALIAS(local)                                                   \
    switch (_coroutine_param._coroutine_location)                             \
    {                                                                         \
        _coroutine_SWITCHCASE(locations)                                      \
        case _coroutine_BEGIN_ ## routine ## _enum:                           \
            break;                                                            \
        default:                                                              \
            assert(0 && "Corrupted internal coroutine state");                \
    }

/// \def END_COROUTINE
/// \brief end a coroutine that was started with `DEFINE_COROUTINE`
#define END_COROUTINE                                                         \
     TERMINATE_COROUTINE_SUCCESSFULLY();                                      \
}


/// \brief starts two coroutines more or less in lockstep
/// \details If the coroutines are member methods, also `RUN_COROUTINES` has to
/// be called within a member method (otherwise, a namespace error will be
/// generated).
/// \param routine1    the first coroutine to be started (defined with
///                    `DEFINE_COROUTINE`)
/// \param param1      actual parameter list of routine1, as a boost sequence:
///                    `(value1) (value2) (value3)` etc.
/// \param final1      statement to be executed if routine1 terminates first
/// \param routine2    the second coroutine to be started (defined with
///                    `DEFINE_COROUTINE`)
/// \param param2      actual parameter list of routine2, as a boost sequence:
///                    `(value1) (value2) (value3)` etc.
/// \param final2      statement to be executed if routine2 terminates first
/// \param shared_type type of the data shared between the two coroutines
/// \param shared_init initial value of the shared data, as a boost sequence:
///                    `(value1) (value2) (value3)` etc.
#define RUN_COROUTINES(routine1, param1, final1, routine2, param2, final2,    \
                                                    shared_type, shared_init) \
        do                                                                    \
        {                                                                     \
            _coroutine_ ## routine1 ## _struct _coroutine_local1 =            \
                _coroutine_ ## routine1 ## _struct(BOOST_PP_SEQ_ENUM(param1));\
            _coroutine_ ## routine2 ## _struct _coroutine_local2 =            \
                _coroutine_ ## routine2 ## _struct(BOOST_PP_SEQ_ENUM(param2));\
            shared_type _coroutine_shared_data =                              \
                                           { BOOST_PP_SEQ_ENUM(shared_init) };\
            for (std::size_t _coroutine_allowance = 1;; )                     \
            {                                                                 \
                coroutine::_coroutine_result_t _coroutine_result =            \
                        _coroutine_ ## routine1 ## _func(_coroutine_allowance,\
                                                     _coroutine_local1,       \
                                                     _coroutine_shared_data); \
                if (coroutine::_coroutine_CONTINUE == _coroutine_result)      \
                {                                                             \
                    assert(0 != _coroutine_allowance);                        \
                    _coroutine_allowance = 2;                                 \
                }                                                             \
                else if (coroutine::_coroutine_TERMINATE == _coroutine_result)\
                {                                                             \
                    {  final1;  }                                             \
                    break;                                                    \
                }                                                             \
                else                                                          \
                {                                                             \
                    assert(coroutine::_coroutine_ABORT == _coroutine_result); \
                    _coroutine_allowance = 0;                                 \
                }                                                             \
                _coroutine_result = _coroutine_ ## routine2 ## _func(         \
                                     _coroutine_allowance, _coroutine_local2, \
                                                     _coroutine_shared_data); \
                if (coroutine::_coroutine_CONTINUE == _coroutine_result)      \
                {                                                             \
                    assert(0 != _coroutine_allowance);                        \
                }                                                             \
                else if (coroutine::_coroutine_TERMINATE == _coroutine_result)\
                {                                                             \
                    {  final2;  }                                             \
                    break;                                                    \
                }                                                             \
                else                                                          \
                {                                                             \
                    assert(coroutine::_coroutine_ABORT == _coroutine_result); \
                    _coroutine_allowance = 0;                                 \
                }                                                             \
            }                                                                 \
        }                                                                     \
        while (0)


/// \brief a `while` loop where every iteration incurs one unit of work
/// \details A `COROUTINE_WHILE` may be interrupted at the end of an iteration
/// to allow the other coroutine to catch up.
/// \param location  a unique interrupt location (one of the locations given as
///                  parameter to `DEFINE_COROUTINE`)
/// \param condition the while condition
/* The pattern of nested do { } while loops and if ... else statements used in
the macro implementation should generate error messages if there is an
unmatched COROUTINE_WHILE or END_COROUTINE_WHILE. */
#define COROUTINE_WHILE(location, condition)                                  \
    do                                                                        \
    {                                                                         \
        if (1)                                                                \
        {                                                                     \
            if (1)                                                            \
            {                                                                 \
                goto _coroutine_ ## location ## _label;                       \
                for ( ;; )                                                    \
                {                                                             \
                    if (0 == --_coroutine_allowance)                          \
                    {                                                         \
                        _coroutine_param._coroutine_location =                \
                                            _coroutine_ ## location ## _enum; \
                        return coroutine::_coroutine_CONTINUE;                \
                    }                                                         \
                    _coroutine_ ## location ## _label:                        \
                    if (!(condition))  break;


/// \brief ends a loop started with `COROUTINE_WHILE`
#define END_COROUTINE_WHILE                                                   \
                }                                                             \
            }                                                                 \
            else  assert(0 && "mismatched if/else pair");                     \
        }                                                                     \
        else  assert(0 && "mismatched if/else pair");                         \
    }                                                                         \
    while (0)


/// \brief a `for` loop where every iteration incurs one unit of work
/// \details A `COROUTINE_FOR` may be interrupted at the end of an iteration
/// to allow the other coroutine to catch up.
/// \param location  a unique interrupt location (one of the locations given as
///                  parameter to `DEFINE_COROUTINE`)
/// \param init      the for initialiser expression
/// \param condition the for condition
/// \param update    the for update expression (executed near the end of each
///                  iteration)
#define COROUTINE_FOR(location, init, condition, update)                      \
    do                                                                        \
    {                                                                         \
        (init);                                                               \
        if (1)                                                                \
            do                                                                \
            {{                                                                \
                goto _coroutine_ ## location ## _label;                       \
                for( ;; )                                                     \
                {                                                             \
                    (update);                                                 \
                    if (0 == --_coroutine_allowance)                          \
                    {                                                         \
                        _coroutine_param._coroutine_location =                \
                                            _coroutine_ ## location ## _enum; \
                        return coroutine::_coroutine_CONTINUE;                \
                    }                                                         \
                    _coroutine_ ## location ## _label:                        \
                    if (!(condition))  break;


/// \brief ends a loop started with `COROUTINE_FOR`
#define END_COROUTINE_FOR                                                     \
                }                                                             \
            }}                                                                \
            while (0);                                                        \
        else  assert(0 && "mismatched if/else pair");                         \
    }                                                                         \
    while (0)


/// \brief a `do { } while` loop where every iteration incurs one unit of work
/// \details A `COROUTINE_DO_WHILE` may be interrupted at the end of an
/// iteration to allow the other coroutine to catch up.
/// Note that one has to specify the condition at the beginnin of the loop,
/// even though it will not be tested before the first iteration.
/// \param location  a unique interrupt location (one of the locations given as
///                  parameter to `DEFINE_COROUTINE`)
/// \param condition the while condition
#define COROUTINE_DO_WHILE(location,condition)                                \
    do                                                                        \
        if (1)                                                                \
        {{                                                                    \
            goto _coroutine_ ## location ## _do_while_begin;                  \
            do                                                                \
            {                                                                 \
                if (0 == --_coroutine_allowance)                              \
                {                                                             \
                    _coroutine_param._coroutine_location =                    \
                                            _coroutine_ ## location ## _enum; \
                    return coroutine::_coroutine_CONTINUE;                    \
                }                                                             \
                _coroutine_ ## location ## _label:                            \
                if (!(condition))  break;                                     \
                _coroutine_ ## location ## _do_while_begin:


/// \brief ends a loop started with `COROUTINE_DO_WHILE`
#define END_COROUTINE_DO_WHILE                                                \
            }                                                                 \
            while (1);                                                        \
        }}                                                                    \
        else  assert(0 && "mismatched if/else pair");                         \
    while (0)


/// \brief terminate the pair of coroutines successfully
/// \details If called in a coroutine, both coroutines will immediately
/// terminate.  The final statement (in `RUN_COROUTINES`) of the one that
/// called `TERMINATE_COROUTINE_SUCCESSFULLY` will be executed.
#define TERMINATE_COROUTINE_SUCCESSFULLY()                                    \
        do                                                                    \
        {                                                                     \
            return coroutine::_coroutine_TERMINATE;                           \
            (void) _coroutine_allowance;                                      \
            (void) _coroutine_param._coroutine_location;                      \
        }                                                                     \
        while (0)


/// \brief indicates that this coroutine gives up control to the other one
/// \details In a situation where one coroutine finds that the other should run
/// to completion, it calls `ABORT_THIS_COROUTINE`.
#define ABORT_THIS_COROUTINE()                                                \
        do                                                                    \
        {                                                                     \
            return coroutine::_coroutine_ABORT;                               \
            (void) _coroutine_allowance;                                      \
            (void) _coroutine_param._coroutine_location;                      \
        }                                                                     \
        while (0)


/// \brief indicates that the other coroutine should give up control
/// \details In a situation where one coroutine finds that the other should
/// **no longer** run, it calls `ABORT_OTHER_COROUTINE`.
#define ABORT_OTHER_COROUTINE()                                               \
        do                                                                    \
        {                                                                     \
            _coroutine_allowance = 0;                                         \
            (void) _coroutine_param._coroutine_location;                      \
        }                                                                     \
        while (0)

#endif // ifndef _COROUTINE_H
