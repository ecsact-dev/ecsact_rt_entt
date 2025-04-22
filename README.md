<p align="center">
	<img src="https://ecsact.dev/assets/logo.svg" width="300" />
</p>

# EnTT Ecsact Runtime Implementation

[Ecsact](https://ecsact.dev) runtime built with [EnTT](https://github.com/skypjack/entt).

## System Views

In the simplest cases an Ecsact system lines up 1:1 with an EnTT view.

```ecsact
package example;
component Health { f32 value; }
component Invincible;
system Gravity { readwrite Position; exclude Weightless; }
```

```cpp
entt::basic_view<entt::entity, entt::get_t<example::Health>, entt::exclude_t<example::Invincible>>
```

Since `adds` implies `exclude` and `removes` implies `include` even `adds`/`removes` systems are _almost_ 1:1.

```ecsact
package example;
component Health { f32 value; }
component Healing;
action StartHealing { include Health; adds Healing; }
action StopHealing { include Health; removes Healing; }
```

```cpp
// EnTT view for StartHealing
entt::basic_view<entt::entity, entt::get_t<example::Health>, entt::exclude_t<example::Healing>>
// EnTT view for StopHealing
entt::basic_view<entt::entity, entt::get_t<example::Health, example::Healing>, entt::exclude_t<>>
```

## Association with entity fields

Ecsact association requires us to handle systems and entities a little differently. I will explain the differences here.

* multiple views per system
* multiple internal components per components with association fields

In the examples below we'll assume the Ecsact file below and the following variables:
* `r` refers to an `entt::registry`
* `basic_view` refers to `entt::baic_view`
* `get_t` refers to `entt::get_t`
* `assoc` refers to `ecsact::entt::assoc`


```ecsact
component Power { f32 value; }
component Attacker { entity target; }
component Health { f32 value; }

system DoDamage {
	readonly Power;
	readonly Health;
	readonly Attacker with target {
		readwrite Health;
	}
}
```

### Adding/Updating with entity fields

When adding a component with associated fields we must also add internal components to 'group' or 'bucket' entities together based on the association. This means that whenever there is an add or update we're actually adding _multiple_ components under the hood.

```cpp
auto add_component(auto entity, Attacker attacker) -> void {
    // add original component
    r.emplace(entity, attacker);

    // add internal component if we meet the DoDamage target association conditions
    if(r.all_of<Health>(attacker.target)) {
        r.emplace(attacker.target, assoc<DoDamage, 0>{});
    }
}

auto update_component(auto entity, Attacker attacker) -> void {
    auto before = r.get<Attacker>(entity);
    r.emplace_or_replace(entity, attacker);

    // if the target (entity field) has changed we must remove the old internal
    // association component and add it to our new target
    if(before.target != attacker.target) {
        r.erase<assoc<DoDamage, 0>>(before.target);
        
        // but we only do so if it has the target association conditions!
        if(r.all_of<Health>(attacker.target)) {
            r.emplace(attacker.target, assoc<DoDamage, 0>{});
        }
    }
}
```

The adding/updating/removing Health also gets more complicated.

```cpp
auto add_component(auto entity, Health health) -> void {
    r.emplace(entity, health);
    
    for(auto attacker_entity : r.view<Attacker>()) {
        auto attacker = r.get<Attacker>();
        if(attacker.target == entity) {
            r.emplace(attacker_entity, assoc<DoDamage,0>{});
            break;
        }
    }
}


auto remove_component<Health>(auto entity) -> void {
    r.erase<Health>(entity);
    
    for(auto attacker_entity : r.view<Attacker>()) {
        auto attacker = r.get<Attacker>();
        if(attacker.target == entity) {
            r.erase<assoc<DoDamage,0>>(attacker_entity);
            break;
        }
    }
}
```

### Iteration

```cpp
// EnTT views for DoDamage
basic_view<entity, get_t<Power>, get_t<Health>, get_t<Attacker>>
basic_view<entity, get_t<Health>, assoc<DoDamage, 0>>
```

Iteration for `DoDamage` is less straight forward as a regular system. Instead of iterating over 1 view we will be iterating over both and hopefully in an optimal succinct fashion.


The inefficient way first:
```cpp
auto main_view = r.view<Power, Health, Attacker>();
for(auto entity : main_view) {
    auto attacker = main_view.get<Attacker>(entity);
    auto assoc_view = r.view<Health, assoc<DoDamage, 0>>();
    for(auto assoc_entity : assoc_view) {
        if(assoc_entity == attacker.entity) {
            // We found the associated entity! Call the system implementation
            system_impl(...);
            break;
        }
    }
}
```

This clearly is unoptimal. For every entity iteration we iterate over a second view. Ideally we can iterate each one by one, side by side.

```cpp
auto main_view = r.view<Power, Health, Attacker>();
auto assoc_view = r.view<Health, assoc<DoDamage, 0>>();

auto main_view_itr = main_view.begin();
auto assoc_view_itr = assoc_view.begin();

for(;;) {
    if(main_view_itr == main_view.end()) break;
    if(assoc_view_itr == assoc_view.end()) break;

    auto main_entity = *main_view_itr;
    auto assoc_entity = *assoc_view_itr;
    
    // Can we assume that the main entity and the assoc entity match?
    system_impl(...);

    ++main_view_itr;
    ++assoc_view_itr;
}
```

Is there a way to we assmume the view iteration of both the `main_view` and `assoc_view` can be aligned? I'm not sure. Possibly through sorting the views and adding some extra internal component to the `main_view` so that the `main_view` and `assoc_view` match in length.

If the above is not possible we could add a little bit of checking like so:

```cpp
auto main_view = r.view<Power, Health, Attacker>();
auto assoc_view = r.view<Health, assoc<DoDamage, 0>>();

auto main_view_itr = main_view.begin();
auto assoc_view_itr = assoc_view.begin();

for(;;) {
    if(main_view_itr == main_view.end()) break;

    auto main_entity = *main_view_itr;
    auto attacker = main_view.get<Attacker>(main_entity);
    
    while(assoc_view_itr != assoc_view.end()) {
        // keep iterating until we found our attacker target entity
        if(*assoc_view_itr == attacker.target) break;
        ++assoc_view_itr;
        
        // TODO: if we reach the end of the assoc_view_itr we have to restart and
        //       make sure we're iterating only up until where we started to prevent
        //       infinite loops.
        // if(assoc_view_itr == assoc_view.end()) { restart! }
    }

    if(assoc_view_itr == assoc_view.end()) break;

    auto assoc_entity = *assoc_view_itr;
    system_impl(...);
    ++main_view_itr;
}
```

This does introduce a second set of iteration but would atleast guarantee we're only running our system implementation on the associated pair of entities.

## Association indexed fields

Association with indexed fields is similar to the entity fields except additional storage for the same types must be created.

TODO: write about this

```ecsact
component OnFire;
component Health { f32 value; }
component GridCell { i32 x; i32 y; }
component WithinCell {
    GridCell.x x;
    GridCell.y y;
}

// When an entity with 'OnFire' is in a cell do damage to all other entities in
// the same cell
system BurnEveryoneInCell {
    include OnFire;
    readonly WithinCell with x,y {
        readwrite Health;
    }
}
```

### Adding/Updating with indexed fields

Adds and updates need to add to a _different_ EnTT storage than the default when using indexed fields. This means only slight changes need to be done to add/update/remove and in some cases requires the indexed fields to be passed in directly. See below:

```cpp
auto add_component(auto entity, GridCell grid_cell) -> void {
    r.emplace(entity, grid_cell); // simple!

    // make sure the associated field storage is allocated
    // this storage will be used for every `WithinCell` with the x/y value
    // being the same as our `GridCell`
    auto hash = storage_hash<WithinCell>(grid_cell.x, grid_cell.y);
    r.storage<WithinCell>(hash);
}

auto add_component(auto entity, WithinCell within_cell) -> void {
    auto hash = storage_hash<WithinCell>(within_cell.x, within_cell.y);
    auto storage = r.storage<WithinCell>(hash);
    storage.push(entity, within_cell); // storage.push is the same as r.emplace
}

auto update_component(auto entity, WithinCell within_cell, i32 x, i32 y) -> void {
    // updating a component now requires the indexed fields to be passed in
    // the reason being is we must remove the `WithinCell` from the storage of
    // the previous indexed fields

    if(within_cell.x != x || within_cell.y != y) {
        // delete from old storage
        auto prev_hash = storage_hash<WithinCell>(x, y);
        auto prev_storage = r.storage<WithinCell>(prev_hash);
        prev_storage.erase(entity);

        // add to new storage
        auto hash = storage_hash<WithinCell>(within_cell.x, within_cell.y);
        auto storage = r.storage<WithinCell>(hash);
        storage.push(entity, within_cell);
    } else {
        auto hash = storage_hash<WithinCell>(within_cell.x, within_cell.y);
        auto storage = r.storage<WithinCell>(hash);
        // update value! for `WithinCell` this wouldn't really happen because
        // all of its fields as association fields, but in the case where a type
        // had other fields this would be important
        storage.get<WithinCell>(entity) = within_cell;
    }
}

auto remove_component<WithinCell>(auto entity, i32 x, i32 y) -> void {
    // removing a component now also requires the indexed fields to be passed
    // in-order to remove from the correct storage
    auto hash = storage_hash<WithinCell>(x, y);
    auto storage = r.storage<WithinCell>(hash);
    storage.erase(entity);
}
```

You might have noticed that since we have different EnTT storage containers for the same component but with different values that you can have an entity with _multiple_ components of the same type.

```cpp
auto entity = r.create();
add_component(entity, WithinCell{0, 0});  // valid
add_component(entity, WithinCell{0, 1});  // also valid!
add_component(entity, WithinCell{2, -3}); // also valid!
```

This enables entities to be associated with multiple buckets. In a simple grid you could imagine that an entity with a large collision box would certainly be considered 'within' multiple cells.

### Iteration

Since we have unique hashed storage for the associated fields (`WithinCell` `x` and `y`) we can construct an `entt::runtime_view` with the storage based on our runtime value. Runtime views are more expensive than regular views, but it would be more expensive for us to check the matching value at runtime.

```cpp
auto main_view = r.view<OnFire, WithinCell>();
for(auto entity : main_view) {
    auto on_fire_cell = main_view.get<WithinCell>(entity);
    auto within_cell_storage_hash = storage_hash<WithinCell>(on_fire_cell.x, on_fire_cell.y);

    auto assoc_view = entt::runtime_view{};
    assoc_view.iterate(r.storage<WithinCell>(within_cell_storage_hash));
    assoc_view.iterate(r.storage<Health>());

    for(auto assoc_entity : assoc_view) {
        system_impl(...);
    }
}
```

...and I wish it was that simple. Unfortunately our `main_view` cannot simply use `WithinCell`'s default storage. We have a unique storage for every possible value of `WithinCell`'s indexed fields. Because of that we need to a way to retrieve all the possible storages and iterate over that. For that we create a storage for our storage. Its the storage storage.

```cpp
template<typename T>
struct storage_storage {
    // storage hashes for type T
    // NOTE: a vector on a component sounds kind of bad - not sure if there is
    // a better way though
    std::vector<uin64_t> storage_hashes;

    auto add_hash(uint64_t) -> void;
    auto remove_hash(uint64_t) -> void;
};
```

Now in every add/update we must update the `storage_storage` for `WithinCell`.

```cpp
auto add_component(auto entity, GridCell grid_cell) -> void {
    // ... stuff before ...

    // nothing needs to change here
}

auto add_component(auto entity, WithinCell within_cell) -> void {
    // ... stuff before ...
    
    auto& storage_storage = r.emplace_or_replace<storage_storage<WithinCell>>(entity);
    auto hash = storage_hash<WithinCell>(within_cell.x, within_cell.y);
    storage_storage.storage_hashes.emplace_back(hash);
}

auto update_component(auto entity, WithinCell within_cell, i32 x, i32 y) -> void {
    // ... stuff before ...
    
    if(/* changed */) {
        auto& storage_storage = r.emplace_or_replace<storage_storage<WithinCell>>(entity);
        auto prev_hash = storage_hash<WithinCell>(x, y);
        storage_storage.remove_hash(prev_hash);

        auto hash = storage_hash<WithinCell>(within_cell.x, within_cell.y);
        r.emplace(entity, storage_storage<WithinCell>{hash});
        storage_storage.add_hash(hash);
    }
}

auto remove_component<WithinCell>(auto entity, i32 x, i32 y) -> void {
    // ... stuff before ...

    auto& storage_storage = r.emplace_or_replace<storage_storage<WithinCell>>(entity);
    auto hash = storage_hash<WithinCell>(x, y);
    storage_storage.remove_hash(hash);
}
```

Now with that our of the way we can introduce our `storage_storage` to our iteration.


```cpp
auto main_view = r.view<OnFire, storage_storage<WithinCell>>();
for(auto entity : main_view) {
    auto storage_storage = main_view.get<storage_storage<WithinCell>>(entity);
    for(auto hash : storage_storage.storage_hashes) {
        auto within_cell_storage = r.storage<WithinCell>(hash);
        // NOTE: storage.get is much less efficient than a view.get
        auto on_fire_cell = within_cell_storage.get<WithinCell>(entity);

        for(auto entity : main_view) {
            auto on_fire_cell = main_view.get<WithinCell>(entity);
            auto within_cell_storage_hash = storage_hash<WithinCell>(on_fire_cell.x, on_fire_cell.y);

            auto assoc_view = entt::runtime_view{};
            assoc_view.iterate(r.storage<WithinCell>(within_cell_storage_hash));
            assoc_view.iterate(r.storage<Health>());

            for(auto assoc_entity : assoc_view) {
                system_impl(...);
            }
        }
    }
}
```

This introduces two slow downs:

1) we're accessing the value of `WithinCell` in the main view with a `storage.get` instead of a `view.get`
2) we're doing an additional layer of iteration - this is only a minor drawback as the iteration count will generally be quite small

## Combining Entity and Indexed fields strategy (the holy grail)

TODO: write about this
