//     Support demand-driven computation
//
// Code does not include statements supporting trace display
// Note crucial interrelationship between synchronized, wait, notify
// Methods available to Stream Objects are
//   put: send a Token Object to a consumer
//   get: get a Token Object from a producer
import java.util.*;
import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

// Database items - the identity is the thread, first is the token number
// that the corresponding consumer should see next
class ConnectEntry {
   Thread thread;
   int first;  // next position from which a token is needed

   public ConnectEntry (Thread t, int f) { 
      thread = t; 
      first = f; 
   }
}

abstract class PipeStream implements Runnable {
   int BUFFER_SIZE = 5, first_pos, last_pos, current;
   Vector <Object> v;
   Thread runner = null;
   boolean get_waiting = false, put_waiting = false;
   Hashtable<Thread,ConnectEntry> connections;
   PC pc;

   public PipeStream (PC pc)  { 
      v = new Vector <Object> ();   // the token buffer
      connections =  new Hashtable<Thread,ConnectEntry>();  // connect database
      first_pos = 0;  // stream token number of 1st token in buffer
      last_pos = -1;  // stream token number of last token in buffer
      current = -1;   // current last token seen by any consumer
      this.pc = pc;   // note: last token may not have been requested yet
   }
   
   // Check to see whether any connected consumers need the first token
   // in the buffer and remove that token if they do not.
   public void removeFirst (Thread thread) {
      boolean flag = false;
      Enumeration e = connections.elements();
      while (e.hasMoreElements()) {
	 ConnectEntry c = (ConnectEntry)e.nextElement();
	 if (c.thread == thread) continue;
	 if (first_pos == c.first) { flag = true; break; }
      }
      if (!flag) {
	 v.remove(0);
	 first_pos++;
      }
   }

   // Print the contents of the buffer to the applet
   public void printBufferToApplet () {
      String str = "";
      for (int i=0 ; i < v.size(); i++) str += v.get(i)+" ";
      pc.vecdisp.setText(str);
   }

   // syncronized does the same thing as 
   // sem_wait(&mutex);
   // ...
   // sem_post(&mutex);
   // or
   // pthread_mutex_lock(&lock);
   // ...
   // pthread_mutex_unlock(&lock);
   synchronized public void put (Object t) {
      // try { wait(); } catch (Exception e) { } does the same thing as
      // sem_wait(&empty);
      // or 
      // pthread_cond_wait(&notifier, &lock);
      while (v.size() > BUFFER_SIZE) {
	 try { put_waiting = true; wait(); } catch (Exception e) { }
      }
      put_waiting = false;
      v.add(t);
      last_pos++;
      // notifyAll() does the same thing as
      // lots of sem_post(&full);
      // or
      // pthread_cond_broadcast(&notifier);
      if (get_waiting) notifyAll ();
      printBufferToApplet();
   }

   // Get function: returns the token in the stream
   //   0. Start the producing thread, if necessary
   //   1. Check whether the requesting thread is connected, return if not
   //   2. Check whether the database has entries, return if not
   //   3. Check whether the requesting thread is in the database, return if not
   //   4. Check whether the buffer is empty, wait if so
   //   5. Store a reference to the gotten token in obj and set current
   //      to the maximum of 'current' and the 'first' of the consumer thread
   //   6. Check whether there are any pending demands for obj.
   //      if not, increment first_pos and remove the token from the buffer
   //   7. Increment first of the thread's ConnectEntry (next token to request)
   //   8. If the producer is waiting, wake it up
   //   9. Print the contents of the buffer to the applet
   synchronized public Object get () {  // synchronized as above for put
      Object obj;
      ConnectEntry ce;

      if (runner == null) {
         runner = new Thread(this);
         runner.start();
      }

      if (connections == null || connections.isEmpty()) {
	 System.out.println("Empty Hash Table");
	 return null;
      }

      Thread thread = Thread.currentThread();
      if ((ce = (ConnectEntry)connections.get(thread)) == null) {
         System.out.println("Not a registered user");
	 return null;
      }

      // try { wait(); } catch (Exception e) { } does the same as
      // sem_wait(&full);
      // or 
      // pthread_cond_wait(&notifier, &lock);
      while (v.isEmpty()) {
	 try { get_waiting = true; wait(); } catch (Exception e) { }
      }
      get_waiting = false;

      if (ce.first > last_pos) return null;

      obj = v.get(ce.first - first_pos);
      current = (ce.first > current) ? ce.first : current;
      
      if (ce.first == first_pos) removeFirst(thread);
      ce.first++;
      // notify() does the same as
      // sem_post(&empty);
      // or
      // pthread_cond_signal(&notifier); 
      if (put_waiting) notify();

      printBufferToApplet();
      return obj;
   }

   synchronized public void connect () {
      Thread thread =  Thread.currentThread();

      if (connections.get(thread) != null)
	 System.out.println("Already connected\n");
      else
	 connections.put(thread, new ConnectEntry(thread, current+1));
   }

   synchronized public void cancel () { }

   synchronized public void disconnect () { }
}

class Producer extends PipeStream {
   int n = 1;
   PC pc;
   String id;

   Producer (PC pc, String id) { 
      super(pc);
      this.id = id; 
      this.pc = pc;
   }

   public void run () {
      while (true) {  put(new Integer(n++));  }
   }
}

class Consumer extends Thread {
   PipeStream stream;
   String id;
   PC pc;
   int s;
   Object ret_value;
   String str[]=new String[] { "connect", "disconnect", "cancel", "getNext" };

   public Consumer (String id, PipeStream s, PC pc) { 
      this.id = id; 
      this.pc = pc; 
      stream = s;
   }

   // The Stream class needs a thread to authorize on so Consumer calls to 
   // connect etc. are diverted to the run method below.
   synchronized public void run () {
      while (true) {
         try { wait(); } catch (Exception e) {}
         try {
            switch (s) {
            case 0: 
               stream.connect(); 
               pc.msg.setText(id+": connected");
               break;
            case 1: 
               stream.disconnect(); 
               pc.msg.setText(id+": disconnected");
               break;
            case 2: 
               stream.cancel(); 
               pc.msg.setText(id+": canceled");
               break;
            case 3: 
	       ret_value = stream.get();
               pc.msg.setText("");
               break;
            }
         } catch (Exception f) {
            pc.msg.setText("["+id+" (getNext)]: ");
	    f.printStackTrace();
            ret_value = null;
         }
         notify();
      }
   }

   synchronized void connect ( ) {
      s = 0;
      notify();
      try { wait(); } catch (Exception e) {}
   }

   synchronized void cancel () {
      s = 2;
      notify();
      try { wait(); } catch (Exception e) {}
   }

   synchronized void disconnect () {
      s = 1;
      notify();
      try { wait(); } catch (Exception e) {}
   }

   synchronized public Object getNext() {
      s = 3;
      notify();
      try { wait(); } catch (Exception e) {}
      return ret_value;
   }
}

class PC extends Frame implements ActionListener {
   Consumer consumer[] = new Consumer[4];
   Producer p;
   String opr[]=new String[] { "Connect", "Consume", "Disconnect", "Cancel" };
   String str[]=new String[] { "A", "B", "C", "D" };
   JButton button[][] = new JButton[4][4];
   JTextField text[] = new JTextField[4], msg, vecdisp;

   PC() {
      Panel r = new Panel();
      r.setLayout(new GridLayout(4,4));
      for (int i=0 ; i < 4 ; i++) {
         for (int j=0 ; j < 4 ; j++) {
            r.add(button[i][j] = new JButton(opr[i]+" "+str[j]));
            button[i][j].addActionListener(this);
         }
      }
      add("North", r);
      Panel q = new Panel();
      q.setLayout(new GridLayout(4,1));
      for (int i=0 ; i < 4 ; i++) {
         r = new Panel();
         r.setLayout(new BorderLayout(1,2));
         r.add("West", new Label(str[i]+":", Label.RIGHT));
         r.add("Center", text[i] = new JTextField());
         q.add(r);
      }
      add("Center", q);

      q = new Panel();
      q.setLayout(new GridLayout(4,1));
      q.add(new JLabel(" "));
      r = new Panel();
      r.setLayout(new BorderLayout());
      r.add("West", new JLabel(" Msgs: "));
      r.add("Center", msg = new JTextField());
      q.add(r);
      q.add(new JLabel(" "));
      r = new Panel();
      r.setLayout(new BorderLayout());
      r.add("West", new JLabel("vector:"));
      r.add("Center", vecdisp = new JTextField());
      q.add(r);
      add("South", q);
      p = new Producer(this, "P");
      for (int i=0 ; i < 4 ; i++) {
         consumer[i] = new Consumer(str[i], p, this);
         consumer[i].start();
      }
   }

   public void actionPerformed (ActionEvent evt) {
      int i=0;
      try {
         for (i=0 ; i < 4 ; i++) {
            if (evt.getSource() == button[0][i]) {
               consumer[i].connect();
            } else if (evt.getSource() == button[1][i]) {
	       Object obj = consumer[i].getNext();
	       if (obj != null) {
		  int numb = ((Integer)obj).intValue();
		  text[i].setText(text[i].getText()+numb+" ");
	       }
            } else if (evt.getSource() == button[2][i]) {
               consumer[i].disconnect();
            } else if (evt.getSource() == button[3][i]) {
               consumer[i].cancel();
            } 
         }
      } catch (NullPointerException e) {
         msg.setText("["+str[i]+" (getNext)]: not connected");
	 e.printStackTrace();
      }
   }
}

public class PipedStreams extends Applet implements ActionListener {
   JButton go;

   public void init () {
      setLayout(new BorderLayout());
      add("Center", go = new JButton("Applet"));
      go.addActionListener(this);
   }

   public void actionPerformed (ActionEvent evt) {
      // Set up many consumers of the producer of one stream
      // of increasing integers.
      PC pc = new PC();
      pc.setSize(700,300);
      pc.setVisible(true);
   }
}
