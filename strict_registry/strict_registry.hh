#pragma once

#include <type_traits>
#include <boost/mp11.hpp>
#include <ecsact/lib.hh>
#include <entt/entity/registry.hpp>

namespace ecsact::entt {

	/**
	 * Simply wraps EnTT's basic registry but only allows components and views
	 * that are defined by the ecsact package meta info.
	 * 
	 * Due to all compoennts being known at compile time there are a couple
	 * differences between EnTT's registry.
	 * 
	 * 1. an (expensive) copy constructor is available
	 */
	template<::ecsact::package Package, typename Entity>
	class basic_strict_registry {
	public:
		using package = Package;
		using entity_type = Entity;
		using entt_registry_type = ::entt::basic_registry<entity_type>;
		using size_type = typename entt_registry_type::size_type;
		using version_type = typename entt_registry_type::version_type;

	private:
		entt_registry_type _registry;

	public:

		/**
		 * Checks if type T is listd as one of the components in the ecact package.
		 * @returns `true` if T is a component belonging to `package`, `false` 
		 *          otherwise.
		 */
		template<typename T>
		static constexpr bool is_component() {
			using boost::mp11::mp_bind_front;
			using boost::mp11::mp_transform_q;
			using boost::mp11::mp_any;
			using boost::mp11::mp_apply;

			return mp_apply<mp_any, mp_transform_q<
				mp_bind_front<std::is_same, std::remove_cvref_t<T>>,
				typename package::components
			>>::value;
		}

		basic_strict_registry() {
			boost::mp11::mp_for_each<typename package::components>(
				[&]<typename C>(C) {
					static_cast<void>(_registry.template storage<C>());
				}
			);
		}

		explicit basic_strict_registry
			( const basic_strict_registry& other
			)
			: basic_strict_registry()
		{
			other._registry.each([&](auto entity) {
				static_cast<void>(_registry.create(entity));
			});
			boost::mp11::mp_for_each<typename package::components>(
				[&]<typename C>(C) {
					if constexpr(std::is_empty_v<C>) {
						other._registry.template view<C>().each([&](auto entity) {
							_registry.template emplace<C>(entity);
						});
					} else {
						other._registry.template view<C>().each([&](auto entity, C& c) {
							_registry.template emplace<C>(entity, c);
						});
					}
				}
			);
		}

		basic_strict_registry
			( basic_strict_registry&& other
			)
			: _registry(std::move(other._registry))
		{
		}

		basic_strict_registry& operator=
			( basic_strict_registry&& other
			)
		{
			_registry = std::move(other._registry);
			return *this;
		}

		inline ~basic_strict_registry() {}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::size<C>()
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] size_type size() const {
			return _registry.template size<C>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::size()
		 */
		[[nodiscard]] size_type size() const noexcept {
			return _registry.size();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::size()
		 */
		[[nodiscard]] size_type alive() const {
			return _registry.alive();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::reserve
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		void reserve
			( const size_type cap
			)
		{
			_registry.reserve(cap);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::capacity
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] size_type capacity() const {
			return _registry.template capacity<C>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::capacity
		 */
		[[nodiscard]] size_type capacity() const noexcept {
			return _registry.capacity();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::shrink_to_fit
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		void shrink_to_fit() {
			_registry.template shrink_to_fit<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::empty
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] bool empty() const {
			return _registry.template empty<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::empty
		 */
		[[nodiscard]] bool empty() const {
			return _registry.empty();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::data
		 */
		[[nodiscard]] const entity_type* data() const noexcept {
			return _registry.data();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::released
		 */
		[[nodiscard]] entity_type released() const noexcept {
			return _registry.released();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::valid
		 */
		[[nodiscard]] bool valid
			( const entity_type entity
			) const
		{
			return _registry.valid(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::current
		 */
		[[nodiscard]] version_type current
			( const entity_type entity
			) const
		{
			return _registry.current(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::create()
		 */
		[[nodiscard]] entity_type create() {
			return _registry.create();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::create(const entity_type)
		 */
		[[nodiscard]] entity_type create
			( const entity_type hint
			)
		{
			return _registry.create(hint);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::create<It>(It,It)
		 */
		template<typename It>
		void create
			( It first
			, It last
			)
		{
			_registry.create(first, last);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::assign
		 */
		template<typename It>
		void assign
			( It                 first
			, It                 last
			, const entity_type  destroyed
			)
		{
			return _registry.assign(first, last, destroyed);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::release
		 */
		version_type release
			( const entity_type entity
			)
		{
			return _registry.release(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::release
		 */
		version_type release
			( const entity_type entity
			, const version_type version
			)
		{
			return _registry.release(entity, version);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::release
		 */
		template<typename It>
		void release
			( It first
			, It last
			)
		{
			return _registry.release(first, last);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::destroy
		 */
		version_type destroy
			( const entity_type entity
			)
		{
			return _registry.destroy(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::destroy
		 */
		version_type destroy
			( const entity_type entity
			, const version_type version
			)
		{
			return _registry.destroy(entity, version);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::destroy
		 */
		template<typename It>
		void destroy
			( It first
			, It last
			)
		{
			_registry.template destroy<It>(first, last);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::emplace
		 */
		template<typename C, typename... Args>
			requires (is_component<C>())
		decltype(auto) emplace
			( const entity_type  entity
			, Args&&...          args
			)
		{
			return _registry.template emplace<C>(entity, std::forward<Args>(args)...);
		}

		// TODO(zaucy): insert

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::emplace_or_replace
		 */
		template<typename C, typename... Args>
			requires (is_component<C>())
		decltype(auto) emplace_or_replace
			( const entity_type  entity
			, Args&&...          args
			)
		{
			return _registry.template emplace_or_replace<C>(
				entity,
				std::forward<Args>(args)...
			);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::patch
		 */
		template<typename C, typename... Func>
			requires (is_component<C>())
		decltype(auto) patch
			( const entity_type  entity
			, Func&&...          func
			)
		{
			return _registry.template patch<C>(entity, std::forward<Func>(func)...);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::replace
		 */
		template<typename C, typename... Args>
			requires (is_component<C>())
		decltype(auto) replace
			( const entity_type  entity
			, Args&&...          args
			)
		{
			return _registry.template replace<C>(entity, std::forward<Args>(args)...);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::remove
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		size_type remove
			( const entity_type entity
			)
		{
			return _registry.template remove<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::remove
		 */
		template<typename... C, typename It>
			requires (is_component<C>() && ...)
		size_type remove
			( It first
			, It last
			)
		{
			return _registry.template remove<C..., It>(first, last);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::erase
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		void erase
			( const entity_type entity
			)
		{
			_registry.template erase<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::erase
		 */
		template<typename... C, typename It>
			requires (is_component<C>() && ...)
		void erase
			( It first
			, It last
			)
		{
			_registry.template erase<C..., It>(first, last);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::compact
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		void compact() {
			_registry.template compact<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::all_of
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] bool all_of
			( const entity_type entity
			) const
		{
			return _registry.template all_of<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::any_of
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] bool any_of
			( const entity_type entity
			) const
		{
			return _registry.template any_of<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::get
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] const C& get
			( const entity_type entity
			) const
		{
			return _registry.template get<C>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::get
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] C& get
			( const entity_type entity
			)
		{
			return _registry.template get<C>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::get_or_emplace
		 */
		template<typename C, typename... Args>
			requires (is_component<C>())
		[[nodiscard]] decltype(auto) get_or_emplace
			( const entity_type  entity
			, Args&&...          args
			)
		{
			return _registry.template get_or_emplace<C, Args...>(
				entity,
				std::forward<Args>(args)...
			);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::try_get
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] auto try_get
			( const entity_type entity
			) const
		{
			return _registry.template try_get<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::try_get
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] auto try_get
			( const entity_type entity
			)
		{
			return _registry.template try_get<C...>(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::clear
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		void clear() {
			_registry.template clear<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::storage
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] decltype(auto) storage() const {
			return _registry.template storage<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::storage
		 */
		template<typename... C>
			requires (is_component<C>() && ...)
		[[nodiscard]] decltype(auto) storage() {
			return _registry.template storage<C...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::each
		 */
		template<typename Func>
		void each
			( Func&& func
			) const
		{
			_registry.each(std::forward<Func>(func));
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::orphan
		 */
		[[nodiscard]] bool orphan
			( const entity_type entity
			) const
		{
			return _registry.orphan(entity);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::orphans
		 */
		template<typename Func>
		void orphans
			( Func&& func
			) const
		{
			_registry.orphans(std::forward<Func>(func));
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::on_construct
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] auto on_construct() {
			return _registry.template on_construct<C>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::on_update
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] auto on_update() {
			return _registry.template on_update<C>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::on_destroy
		 */
		template<typename C>
			requires (is_component<C>())
		[[nodiscard]] auto on_destroy() {
			return _registry.template on_destroy<C>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::view
		 */
		template<typename... C, typename... E>
			requires (
				(is_component<C>() && ...) &&
				(package::template is_component_v<E> && ...)
			)
		[[nodiscard]]
		::entt::basic_view<Entity, ::entt::exclude_t<E...>, std::add_const_t<C>...> 
		view
			( ::entt::exclude_t<E...> = {}
			) const
		{
			return _registry.template view<C..., E...>();
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::view
		 */
		template<typename... C, typename... E>
			requires (
				(is_component<C>() && ...) &&
				(is_component<E>() && ...)
			)
		[[nodiscard]]
		::entt::basic_view<Entity, ::entt::exclude_t<E...>, std::add_const_t<C>...> 
		view
			( ::entt::exclude_t<E...> = {}
			)
		{
			return _registry.template view<C..., E...>();
		}

		// TODO(zaucy): runtime_view
		// TODO(zaucy): group

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::visit
		 */
		template<typename Func>
		void visit
			( Func&& func
			) const
		{
			return _registry.template visit<Func>(std::forward<Func>(func));
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::sort
		 */
    template
			< typename     Component
			, typename     Compare
			, typename     Sort = ::entt::std_sort
			, typename...  Args
			> requires (
				is_component<Component>()
			)
    void sort
			( Compare&&  compare
			, Sort&&     algo = Sort{}
			, Args&&...  args
			)
		{
			_registry.template sort<Component, Compare, Sort, Args...>(
				std::forward<Compare>(compare),
				std::forward<Sort>(algo),
				std::forward<Args>(args)...
			);
		}

		/**
		 * @copydoc ::entt::basic_registry<entity_type>::sort
		 */
    template<typename To, typename From>
			requires(is_component<To>() && is_component<From>())
    void sort() {
			_registry.template sort<To, From>();
		}

	};

	template<::ecsact::package Package>
	using strict_registry = basic_strict_registry
		< Package
		, ::entt::entity
		>;

}
