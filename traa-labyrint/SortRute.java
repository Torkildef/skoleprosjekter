import java.util.ArrayList;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class SortRute extends Rute{
  SortRute(int x, int y, Labyrint l){
    super(x, y, l);
    setBackground(Color.BLACK);
    //Ikke mulig å rykke på
    setEnabled(false);
  }

  @Override
  public char tilTegn(){
    return '#';
  }

  //Møter veien en sortRute skal den stoppe
  @Override
  public void gaa(Rute forrige, ArrayList<Rute> sti){
    return;
  }

  @Override
  public String toString(){
    return "SortRute: (" + this.kolonne + ", " + this.rad + ")";
  }
}
