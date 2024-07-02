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
