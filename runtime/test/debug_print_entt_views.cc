#include <iostream>
#include <type_traits>
#include <boost/mp11.hpp>
#include "ecsact/entt/system_view.hh"
#include "runtime_test.ecsact.meta.hh"

auto replace_all(std::string& str, std::string_view find, std::string_view replace) {
	auto idx = str.find(find);
	while(idx != std::string::npos) {
		str.replace(idx, find.size(), replace);
		idx = str.find(find, idx + find.size());
	}
}

template<typename T>
auto pretty_type_string() {
	std::string pretty_type = typeid(T).name();

	replace_all(pretty_type, "class ", "");
	replace_all(pretty_type, "struct ", "");
	replace_all(pretty_type, "enum ", "");
	replace_all(pretty_type, "ecsact::entt::detail::", "");
	replace_all(pretty_type, "entt::basic_view", "view");
	replace_all(pretty_type, "runtime_test::", "");
	replace_all(pretty_type, ",", ", ");
	replace_all(pretty_type, "> >", ">>");

	return pretty_type;
}

template<typename S>
auto print_system_views() {
	using boost::mp11::mp_for_each;
	using ecsact::entt::system_view_type;
	using ecsact::entt::system_association_views_type;

	std::cout //
		<< "  " << pretty_type_string<S>() << "\n"
		<< "  | " << pretty_type_string<system_view_type<S>>() << "\n";

	mp_for_each<system_association_views_type<S>>(
		[]<typename V>(V) {
		std::cout //
			<< "  | " << pretty_type_string<V>() << " (association view)\n";
		}
	);
	
	std::cout << "\n";
}

auto main() -> int {
	using boost::mp11::mp_for_each;
	using runtime_test::package;

	std::cout << " ==== [ System Views ] ==== \n";
	mp_for_each<typename package::systems>([]<typename S>(S) {
		print_system_views<S>();
	});

	std::cout << " ==== [ Action Views ] ==== \n";
	mp_for_each<typename package::actions>([]<typename S>(S) {
		print_system_views<S>();
	});

	return 0;
}
