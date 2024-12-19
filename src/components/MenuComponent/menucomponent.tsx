import MenuComponentProps from "./menucomponentprops";

const MenuComponent = ({ name, class: class_, onclick: onclick_, onmouseover: onmouseover_ }: MenuComponentProps) => {
  return (
    <div class={class_!()} onclick={onclick_} onmouseover={onmouseover_}>
      {name}
    </div>
  );
}

export default MenuComponent;