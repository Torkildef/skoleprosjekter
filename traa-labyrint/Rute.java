import java.util.ArrayList;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.concurrent.locks.*;

//Kan ikke lage ruteobjekt direkte
abstract class Rute extends JButton{
  int kolonne;
  int rad;
  Labyrint labyrint;
  int antallTraaderSomGaarVei = 0;
  int blaaFarge = 255;
    Lock laas = new ReentrantLock();

  //naboer i henholdsvis nord, syd, Vest, ost
  protected Rute[] naboer = new Rute[4];
  Rute(int x, int y, Labyrint l){
    this.kolonne = x;
    this.rad = y;
    this.labyrint = l;
    //setText(String.valueOf(tilTegn()));
    //setPreferredSize(new Dimension(10, 10));
  }

  abstract public char tilTegn();

  abstract public void gaa(Rute forrige, ArrayList<Rute> sti);

  abstract public String toString();

  //Starter på en rute og finner vei ut
  public void finnUtvei(){
    ArrayList<Rute> sti = new ArrayList<Rute>();
    gaa(this, sti);
  }

  public void setDesign(){
    laas.lock();
    try{
      this.antallTraaderSomGaarVei++;
      int farge = blaaFarge - antallTraaderSomGaarVei;
      setBackground(new java.awt.Color(100,100,farge));
      if(antallTraaderSomGaarVei>1) setText(String.valueOf(antallTraaderSomGaarVei));
      }
    finally{
      laas.unlock();
    }
  }

  public void nulstillRute(){
    blaaFarge = 255;
    setBackground(Color.WHITE);
    setText("");
    antallTraaderSomGaarVei=0;
  }

  protected Labyrint hentLabyrint(){
    return this.labyrint;
  }

  //Oppgaver som utføres når knappene trykkes ned
  class Velger implements ActionListener {
    @Override
    public void actionPerformed(ActionEvent e){
      hentLabyrint().nullStill();
      finnUtvei();
      hentLabyrint().visVeier();

    }
  }

  void initGUI(){
    addActionListener(new Velger());
  }

}
