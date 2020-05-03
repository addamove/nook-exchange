[@bs.deriving jsConverter]
type itemStatus =
  | [@bs.as 1] Wishlist
  | [@bs.as 2] ForTrade
  | [@bs.as 3] CanCraft
  | [@bs.as 4] CatalogOnly;

let itemStatusToUrl = itemStatus =>
  switch (itemStatus) {
  | Wishlist => "wishlist"
  | ForTrade => "for-trade"
  | CanCraft => "can-craft"
  | CatalogOnly => "catalog"
  };
let urlToItemStatus = url =>
  switch (url) {
  | "for-trade" => Some(ForTrade)
  | "can-craft" => Some(CanCraft)
  | "wishlist" => Some(Wishlist)
  | "catalog" => Some(CatalogOnly)
  | _ => None
  };
let itemStatusToEmoji = itemStatus => {
  switch (itemStatus) {
  | Wishlist => {j|🙏|j}
  | ForTrade => {j|🤝|j}
  | CanCraft => {j|🔨|j}
  | CatalogOnly => {j|🔨|j}
  };
};
let itemStatusToString = itemStatus =>
  switch (itemStatus) {
  | Wishlist => "Wishlist"
  | ForTrade => "For Trade"
  | CanCraft => "Can Craft"
  | CatalogOnly => "Catalog"
  };

type item = {
  status: itemStatus,
  note: string,
  timeUpdated: option(float),
};

let itemToJson = (item: item) => {
  Json.Encode.(
    object_(
      [
        Some(("status", int(itemStatusToJs(item.status)))),
        item.note != "" ? Some(("note", string(item.note))) : None,
      ]
      ->Belt.List.keepMap(x => x),
    )
  );
};

let itemFromJson = json => {
  open Json.Decode;
  let status = (json |> field("status", int))->itemStatusFromJs;
  Belt.Option.map(status, status =>
    {
      status,
      note:
        (json |> optional(field("note", string)))
        ->Belt.Option.getWithDefault(""),
      timeUpdated: json |> optional(field("t", Json.Decode.float)),
    }
  );
};

let getItemKey = (~itemId: int, ~variation: int) => {
  string_of_int(itemId) ++ "@@" ++ string_of_int(variation);
};

let fromItemKey = (~key: string) => {
  let [|itemId, variation|] = key |> Js.String.split("@@");
  (int_of_string(itemId), int_of_string(variation));
};

type t = {
  id: string,
  username: string,
  email: option(string),
  items: Js.Dict.t(item),
  profileText: string,
  enableCatalogCheckbox: bool,
};

let fromAPI = (json: Js.Json.t) => {
  Json.Decode.{
    id: json |> field("uuid", string),
    username: json |> field("username", string),
    email:
      switch (json |> optional(field("email", string))) {
      | Some("") => None
      | Some(email) => Some(email)
      | None => None
      },
    items:
      (json |> field("items", dict(itemFromJson)))
      ->Js.Dict.entries
      ->Belt.Array.keepMap(((itemKey, value)) => {
          Belt.Option.flatMap(
            value,
            value => {
              let (itemId, variant) = fromItemKey(~key=itemKey);
              Item.itemMap
              ->Js.Dict.get(string_of_int(itemId))
              ->Belt.Option.map(item => {
                  let canonicalVariant =
                    Item.getCanonicalVariant(~item, ~variant);
                  (getItemKey(~itemId, ~variation=canonicalVariant), value);
                });
            },
          )
        })
      ->Js.Dict.fromArray,
    profileText:
      (
        json
        |> field("metadata", json =>
             json |> optional(field("profileText", string))
           )
      )
      ->Belt.Option.getWithDefault(""),
    enableCatalogCheckbox:
      (
        json
        |> field("metadata", json =>
             json |> optional(field("enableCatalogCheckbox", bool))
           )
      )
      ->Belt.Option.getWithDefault(false),
  };
};