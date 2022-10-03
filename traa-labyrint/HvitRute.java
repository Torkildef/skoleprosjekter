import java.util.ArrayList;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

class HvitRute extends Rute{
  HvitRute(int x, int y, Labyrint l){
    super(x, y, l);
    setBackground(Color.WHITE);
  }

  @Override
  public char tilTegn(){
    return '.';
  }

  //Går et steg i en retning og lagrer veien i en Arraylist
  @Override
  public void gaa(Rute forrige, ArrayList<Rute> sti){
    for (Rute r : sti){
      if(r.kolonne == this.kolonne){
        if(r.rad == this.rad){
            return;
        }
      }
    }
    ArrayList<Rute> nySti = new ArrayList<>(sti);
    nySti.add(this);
    //Skjekker alle mulige naboer
    for(int i = 0; i < super.naboer.length; i++){
      if(super.naboer[i] != null){
        if(super.naboer[i] != forrige){
          super.naboer[i].gaa(this, nySti);
        }
      }
    }


  }
  @Override
  public String toString(){
    return "HvitRute: (" + this.kolonne + ", " + this.rad + ")";
  }
}
