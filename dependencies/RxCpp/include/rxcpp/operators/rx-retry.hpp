// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_RETRY_HPP)
#define RXCPP_OPERATORS_RX_RETRY_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

    namespace operators {

        namespace detail {

            template<class T, class Observable, class Count>
            struct retry : public operator_base<T> {
                typedef typename std::decay<Observable>::type source_type;
                typedef typename std::decay<Count>::type count_type;
                struct values {
                    values(source_type s, count_type t)
                    : source(std::move(s))
                    , remaining(std::move(t))
                    , retry_infinitely(t == 0) {
                    }
                    source_type source;
                    count_type remaining;
                    bool retry_infinitely;
                };
                values initial;

                retry(source_type s, count_type t)
                    : initial(std::move(s), std::move(t)) {
                }

                template<class Subscriber>
                void on_subscribe(const Subscriber& s) const {

                    typedef Subscriber output_type;
                    struct state_type
                        : public std::enable_shared_from_this<state_type>
                        , public values {
                        state_type(const values& i, const output_type& oarg)
                        : values(i)
                        , source_lifetime(composite_subscription::empty())
                        , out(oarg) {
                        }
                        composite_subscription source_lifetime;
                        output_type out;

                        void do_subscribe() {
                            auto state = this->shared_from_this();

                            state->source_lifetime = composite_subscription();
                            state->out.add(state->source_lifetime);

                            state->source.subscribe(
                                state->out,
                                state->source_lifetime,
                                // on_next
                                [state](T t) {
                                state->out.on_next(t);
                            },
                                // on_error
                                [state](std::exception_ptr e) {
                                if (state->retry_infinitely || (--state->remaining >= 0)) {
                                    state->do_subscribe();
                                } else {
                                    state->out.on_error(e);
                                }
                            },
                                // on_completed
                                [state]() {

                                    // JEP: never appeears to be called?

                                    state->out.on_completed();
                            }
                            );
                        }
                    };

                    // take a copy of the values for each subscription
                    auto state = std::shared_ptr<state_type>(new state_type(initial, s));

                    // start the first iteration
                    state->do_subscribe();
                }
            };

            template<class T>
            class retry_factory {
                typedef typename std::decay<T>::type count_type;
                count_type count;
            public:
                retry_factory(count_type t) : count(std::move(t)) {}

                template<class Observable>
                auto operator()(Observable&& source)
                    ->      observable<typename std::decay<Observable>::type::value_type, retry<typename std::decay<Observable>::type::value_type, Observable, count_type>> {
                    return  observable<typename std::decay<Observable>::type::value_type, retry<typename std::decay<Observable>::type::value_type, Observable, count_type>>(
                        retry<typename std::decay<Observable>::type::value_type, Observable, count_type>(std::forward<Observable>(source), count));
                }
            };

        }

        template<class T>
        auto retry(T&& t)
            ->      detail::retry_factory<T> {
            return  detail::retry_factory<T>(std::forward<T>(t));
        }

    }

}

#endif
