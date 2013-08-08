/*
 * Copyright (c) 2013 Björn Aili
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */
#ifndef FTL_FUNCTOR_H
#define FTL_FUNCTOR_H

#include "../type_functions.h"
#include "../function.h"

namespace ftl {
	// Forward declaration so we can mention applicatives
	template<typename F>
	struct applicative;

	/**
	 * \page functorpg Functor
	 *
	 * Abstraction of contexts that can be mapped to.
	 *
	 * Mathematically, functors are mappings from one category to another,
	 * following a set of well defined laws. What this means in FTL is that
	 * partial types (ie, vector is partial, vector<int> is complete) can often
	 * be made functors by giving a means of mapping a morphism (function) from
	 * the "pure" universe of plain types, to its own internal type universe.
	 *
	 * Ie, \c int is a plain type, \c vector<int> is an \c int "trapped" in the
	 * context of \c vector, and the mapping should apply the function "inside"
	 * that context. The most obvious way of doing so would be to apply the
	 * mapped function to every element in the vector.
	 *
	 * All instances of Functor should follow the laws below (which are part
	 * of the mathematical definition from which this concept is derived):
	 * - **Preservation of identity**
	 *   \code
	 *     map(id, t)          <=> t
	 *   \endcode
	 * - **Preservation of composition**
	 *   \code
	 *     map(compose(f, g), t)  <=> compose(curry(map)(f), curry(map)(g))(t)
	 *   \endcode
	 *
	 * For a detailed description of what exactly an instance needs to
	 * implement, see the ftl::functor interface.
	 *
	 * \see \ref functor (module)
	 */

	/**
	 * \defgroup functor Functor
	 *
	 * Module containg the \ref functorpg concept and related functions.
	 *
	 * \code
	 *   #include <ftl/concepts/functor.h>
	 * \endcode
	 *
	 * \par Dependencies
	 * - \ref typelevel
	 * - <ftl/function.h>
	 */

	/**
	 * \interface functor
	 *
	 * Struct that must be specialised to implement the functor concept.
	 *
	 * When specialising, it is important to remember that the described
	 * interface here is the _default_, and an instance is encouraged to include
	 * additional overloads of the functions for optimization reasons and
	 * similar. For types that cannot be copied, for instance, it makes sense
	 * that all of the functor interface functions accept an rvalue reference.
	 *
	 * \ingroup functor
	 */
	template<typename F_>
	struct functor {
		/// Convenient access to the type `F_` is parametrised on.
		using T = concept_parameter<F>;

		/**
		 * Clean way of referring to differently parametrised `F`s.
		 */
		template<typename U>
		using F = typename re_parametrise<F_,U>::type;

		/**
		 * Maps a function to the contained value(s).
		 *
		 * Default implementation is to invoke `applicative<F>::map`.
		 */
		template<typename Fn, typename U = result_of<Fn(T)>>
		static F<U> map(Fn&& fn, const F<T>& f) {
			return applicative<F_>::map(
					std::forward<Fn>(fn), f);
		}

		/// \overload
		template<typename Fn, typename U = result_of<Fn(T)>>
		static F<U> map(Fn&& fn, F<T>&& f) {
			return applicative<F_>::map(std::forward<Fn>(fn), std::move(f));
		}

		/**
		 * Compile time check whether a type is a functor.
		 *
		 * Because all applicative functors are functors, `F` is an instance
		 * of functor if it is an instance of applicative.
		 */
		static constexpr bool instance = applicative<F>::instance;
	};

	/**
	 * Concepts lite-compatible predicate for functor instances.
	 *
	 * Can be used already for similar purposes by way of SFINAE.
	 *
	 * Example usage:
	 * \code
	 *   template<
	 *       typename F,
	 *       typename = typename std::enable_if<Functor<F>()>::type
	 *   >
	 *   void myFunc(const F& f);
	 * \endcode
	 *
	 * \ingroup functor
	 */
	template<typename F>
	constexpr bool Functor() noexcept {
		return functor<F>::instance;
	}

	/**
	 * Convenience operator for `functor::map`.
	 *
	 * This operator perfectly forwards the parameters, so it works regardless
	 * of r-valueness.
	 *
	 * Example usage:
	 * \code
	 *   MyFunctor<SomeType> foo(
	 *       const MyFunctor<OtherType>& f,
	 *       ftl::function<SomeType,OtherType> fn) {
	 *
	 *       using ftl::operator%;
	 *       return fn % f;
	 *   }
	 *
	 *   // Equivalent operator-less version
	 *   MyFunctor<SomeType> foo(
	 *       const MyFunctor<OtherType>& f,
	 *       ftl::function<SomeType,OtherType> fn) {
	 *
	 *       return functor<MyFunctor<OtherType>>::map(fn, f);
	 *   }
	 * \endcode
	 *
	 * \ingroup functor
	 */
	template<
		typename F,
		typename Fn,
		typename F_ = plain_type<F>,
		typename = typename std::enable_if<Functor<F_>()>::type>
	auto operator% (Fn&& fn, F&& f)
	-> decltype(functor<F_>::map(std::forward<Fn>(fn), std::forward<F>(f))) {
		return functor<F_>::map(std::forward<Fn>(fn), std::forward<F>(f));
	}

	/**
	 * Convenience function object.
	 *
	 * Provided to make it easier to pass `functor::map` as parameter to
	 * higher order functions, as one might otherwise have to wrap such calls
	 * in a lambda to deal with the ambiguity in face of overloads.
	 *
	 * \ingroup functor
	 */
	struct fMap {
		template<typename Fn, typename F, typename F_ = plain_type<F>>
		auto operator() (Fn&& fn, F&& f) const
		-> decltype(functor<F_>::map(std::forward<Fn>(fn), std::forward<F>(f))) {
			return functor<F_>::map(std::forward<Fn>(fn), std::forward<F>(f));
		}
	};

	/**
	 * Compile time instance of fMap.
	 *
	 * Makes it even more convenient to pass `functor::map` as parameter to
	 * higher order functions.
	 *
	 * Example usage:
	 * \code
	 *   template<
	 *       typename F1, typename F2,
	 *       typename T, typename U = result_of<F1(F2,T)>
	 *   >
	 *   U foo(const F1& f1, const F2& f2, const T& t) {
	 *       return f1(f2, t);
	 *   }
	 *
	 *   MyFunctor<SomeType> bar(MyFunctor<SomeType> myFunctor) {
	 *       // Really a no-op, but demonstrates how convenient fmap can be
	 *       return foo(fmap, id, myFunctor);
	 *   }
	 * \endcode
	 *
	 * \ingroup functor
	 */
	constexpr fMap fmap{};

}

#endif
