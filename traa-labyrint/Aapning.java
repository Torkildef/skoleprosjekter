import java.util.ArrayList;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class Aapning extends HvitRute{
  Aapning(int x, int y, Labyrint l){
    super(x, y, l);
    setBackground(Color.WHITE);
  }

  @Override
  public char tilTegn(){
    return '.';
  }

  //Om veien møter apning legger den til utveien i labirinten sin utveiliste
  @Override
  public void gaa(Rute forrige, ArrayList<Rute> sti){
    
    ArrayList<Rute> nySti = new ArrayList<>(sti);
    nySti.add(this);
    setBackground(Color.RED);
    this.labyrint.leggTilUtVei(nySti);
  }

  @Override
  public String toString(){
    return "Aapning: (" + this.kolonne + ", " + this.rad + ")";
  }
}
