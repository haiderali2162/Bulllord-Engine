package org.bulllord.lib;
import android.app.NativeActivity;
import android.app.Activity;
import android.view.View;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.widget.EditText;
import android.content.Context;
import android.content.Intent;
import android.view.inputmethod.InputMethodManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.view.inputmethod.EditorInfo;
import android.text.InputType;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.util.Log;
import org.json.JSONObject;
import org.json.JSONException;
public class BLActivity extends NativeActivity implements BLGoogleIAB.IBillingHandler
{
    public native void textChanged(String text);
	public native void activityReady(Activity activity);
    public native void productPurchased(int code, String recept, String signature);
    public native void validationResult(int code);
	public native void checkUnfulfilled(String productId, String recept, String signature);
    public class BLText extends EditText
    {		
		public BLText(Context context) {
			super(context);
			setSingleLine();
			setBackgroundColor(Color.TRANSPARENT);
			setImeOptions(EditorInfo.IME_ACTION_NONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI);
			setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			addTextChangedListener(new TextWatcher()
			{
				@Override
				public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
				@Override
				public void onTextChanged(CharSequence s, int start, int before, int count)
				{
					final String text = new String(s.toString());
					textChanged(text);
				}
				@Override
				public void afterTextChanged(Editable s) {}
			});
		}		
		@Override
		public boolean onKeyDown(final int pKeyCode, final KeyEvent pKeyEvent) 
		{   
			switch (pKeyCode) 
			{
				case KeyEvent.KEYCODE_BACK:
				case KeyEvent.KEYCODE_ENTER:
					hideTextInput();
					return true;
				default:
					return super.onKeyDown(pKeyCode, pKeyEvent);
			}
		}
    }	
	protected static BLActivity mActivity;
    protected static BLText mInput;
	protected int mKeyMode;
	protected BLGoogleIAB mIab;
	protected boolean mIabAvailable = false;
	@Override
    protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);	
		mActivity = (BLActivity)this;
		mActivity.runOnUiThread(new Runnable()
		{
			@Override
			public void run() {
				mInput = new BLText((Context)mActivity);
				ViewGroup.LayoutParams param = new ViewGroup.LayoutParams(0, 0);
				mActivity.addContentView(mInput, param);
				mInput.setVisibility(View.GONE);
				InputMethodManager imm = (InputMethodManager)mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
				imm.hideSoftInputFromWindow(mInput.getWindowToken(), 0);
			}
		});
		mActivity.getWindow().getDecorView().getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener()
		{
			@Override
			public void onGlobalLayout()
			{				
				View view = mActivity.getWindow().getDecorView();
				view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE|View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
 			}
		});
		mIab = new BLGoogleIAB(this, this);
		activityReady(mActivity);
	}	
	@Override
    protected void onDestroy()
	{
		if (mIab != null)
			mIab.release();
		super.onDestroy();
    }
	@Override
	public void onBillingInitialized()
	{
		mIabAvailable = true;
	}
	@Override
	public void onProductPurchased(String productId, BLTransactionDetails details) 
	{
		productPurchased(0, details.mPurchaseInfo.mResponseData, details.mPurchaseInfo.mSignature);
	}
	@Override
	public void onBillingError(int errorCode, Throwable error) 
	{
		productPurchased(1, "", "");
	}
	@Override
	public void onPurchaseHistoryRestored()
	{
	}
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (!mIab.handleActivityResult(requestCode, resultCode, data)) 
			super.onActivityResult(requestCode, resultCode, data);
	}
	public void setSignature(String signature)
	{
		mIab.setSignature(signature);
	}
	public void purchase(String ProductId)
	{
		if (mIabAvailable)
			mIab.purchase(this, ProductId);
	}
	public void validation()
	{
		if (mIabAvailable)
		{
			if (mIab.verifyPurchaseSignature())
				validationResult(0);
			else
				validationResult(1);
			mIab.consumePurchase();
		}
	}
	public void checkUnfulfilled()
	{
		if (mIab.syncPurchases())
		{
			try
			{
				JSONObject purchase = new JSONObject(mIab.mCurPurchaseData);
				checkUnfulfilled(purchase.getString("productId"), mIab.mCurPurchaseData, mIab.mCurDataSignature);
			}
			catch (Exception e)
			{
				Log.e("GoogleIAB", "Can not parse productId", e);
			}
			mIab.mCurPurchaseData = null;
			mIab.mCurDataSignature = null;
		}
	}
	public void showTextInput(int keymode)
	{
		 mKeyMode = keymode;
		 runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mKeyMode == 0)
					mInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
				else if (mKeyMode ==1)
					mInput.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED);
				else
					mInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
				mInput.setVisibility(View.VISIBLE);
				mInput.requestFocus();
				InputMethodManager imm = (InputMethodManager)mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
				imm.showSoftInput(mInput, 0);
			}
		});
    }
	public void hideTextInput()
	{
		 runOnUiThread(new Runnable() {
			@Override
			public void run() {
				mInput.setText(""); 
				mInput.setVisibility(View.GONE);
				InputMethodManager imm = (InputMethodManager)mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
				imm.hideSoftInputFromWindow(mInput.getWindowToken(), 0);
			}
		});
    }   
	static {
        System.loadLibrary("Bulllord");
    }
}